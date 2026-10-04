"""
SQLite Time-Series Storage for theSpeaker (3 boards)
Provides persistent storage for telemetry data, alerts, and configurations.
"""

import sqlite3
import json
import os
from datetime import datetime, timedelta
from typing import Dict, List, Any, Optional, Tuple
from contextlib import contextmanager
from pathlib import Path

DB_PATH = os.environ.get('DB_PATH', 'data/telemetry.db')

# Ensure data directory exists
Path(DB_PATH).parent.mkdir(parents=True, exist_ok=True)


@contextmanager
def get_db():
    """Get database connection with WAL mode for concurrent access."""
    conn = sqlite3.connect(DB_PATH, timeout=10)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=NORMAL")
    conn.row_factory = sqlite3.Row
    try:
        yield conn
        conn.commit()
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


def init_db():
    """Initialize database tables."""
    with get_db() as conn:
        conn.executescript("""
            CREATE TABLE IF NOT EXISTS telemetry (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp TEXT NOT NULL,
                device_id TEXT NOT NULL,
                signal_name TEXT NOT NULL,
                value REAL NOT NULL,
                created_at TEXT DEFAULT (datetime('now'))
            );

            CREATE INDEX IF NOT EXISTS idx_telemetry_device_time
                ON telemetry(device_id, timestamp DESC);

            CREATE INDEX IF NOT EXISTS idx_telemetry_signal_time
                ON telemetry(signal_name, timestamp DESC);

            CREATE TABLE IF NOT EXISTS alerts (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                rule_id TEXT,
                device_id TEXT,
                signal_name TEXT NOT NULL,
                value REAL NOT NULL,
                threshold REAL NOT NULL,
                condition TEXT NOT NULL,
                severity TEXT NOT NULL DEFAULT 'warning',
                acknowledged INTEGER DEFAULT 0,
                resolved_at TEXT,
                created_at TEXT DEFAULT (datetime('now'))
            );

            CREATE INDEX IF NOT EXISTS idx_alerts_time
                ON alerts(created_at DESC);

            CREATE TABLE IF NOT EXISTS alert_rules (
                id TEXT PRIMARY KEY,
                signal_name TEXT NOT NULL,
                condition TEXT NOT NULL,
                threshold REAL NOT NULL,
                severity TEXT NOT NULL DEFAULT 'warning',
                enabled INTEGER DEFAULT 1,
                hysteresis REAL DEFAULT 2.0,
                created_at TEXT DEFAULT (datetime('now'))
            );

            CREATE TABLE IF NOT EXISTS settings (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL,
                updated_at TEXT DEFAULT (datetime('now'))
            );
        """)


def store_telemetry(device_id: str, readings: Dict[str, float], timestamp: Optional[str] = None):
    """Store telemetry readings in the database."""
    ts = timestamp or datetime.utcnow().isoformat()
    with get_db() as conn:
        rows = [(ts, device_id, signal, value) for signal, value in readings.items()
                if isinstance(value, (int, float))]
        if rows:
            conn.executemany(
                "INSERT INTO telemetry (timestamp, device_id, signal_name, value) VALUES (?, ?, ?, ?)",
                rows
            )


def query_history(
    device_id: Optional[str] = None,
    signal_name: Optional[str] = None,
    from_time: Optional[str] = None,
    to_time: Optional[str] = None,
    limit: int = 1000,
    aggregation: Optional[str] = None,
    interval_seconds: int = 60
) -> List[Dict[str, Any]]:
    """Query historical telemetry data.

    Args:
        device_id: Filter by device
        signal_name: Filter by signal
        from_time: Start time ISO string
        to_time: End time ISO string
        limit: Max rows to return
        aggregation: 'avg', 'min', 'max', or None for raw
        interval_seconds: Aggregation bucket size
    """
    conditions = []
    params = []

    if device_id:
        conditions.append("device_id = ?")
        params.append(device_id)
    if signal_name:
        conditions.append("signal_name = ?")
        params.append(signal_name)
    if from_time:
        conditions.append("timestamp >= ?")
        params.append(from_time)
    if to_time:
        conditions.append("timestamp <= ?")
        params.append(to_time)

    where = f"WHERE {' AND '.join(conditions)}" if conditions else ""

    with get_db() as conn:
        if aggregation and aggregation in ('avg', 'min', 'max'):
            # Time-bucketed aggregation
            query = f"""
                SELECT
                    strftime('%Y-%m-%dT%H:%M:', timestamp) ||
                    printf('%02d', (CAST(strftime('%S', timestamp) AS INTEGER) / {interval_seconds}) * {interval_seconds}) as bucket,
                    device_id,
                    signal_name,
                    {aggregation}(value) as value,
                    COUNT(*) as sample_count
                FROM telemetry
                {where}
                GROUP BY bucket, device_id, signal_name
                ORDER BY bucket DESC
                LIMIT ?
            """
        else:
            query = f"""
                SELECT timestamp, device_id, signal_name, value
                FROM telemetry
                {where}
                ORDER BY timestamp DESC
                LIMIT ?
            """

        params.append(limit)
        rows = conn.execute(query, params).fetchall()
        return [dict(row) for row in rows]


def get_statistics(
    device_id: Optional[str] = None,
    signal_name: Optional[str] = None,
    from_time: Optional[str] = None,
    to_time: Optional[str] = None,
) -> List[Dict[str, Any]]:
    """Get min, max, avg, stddev for signals."""
    conditions = []
    params = []

    if device_id:
        conditions.append("device_id = ?")
        params.append(device_id)
    if signal_name:
        conditions.append("signal_name = ?")
        params.append(signal_name)
    if from_time:
        conditions.append("timestamp >= ?")
        params.append(from_time)
    if to_time:
        conditions.append("timestamp <= ?")
        params.append(to_time)

    where = f"WHERE {' AND '.join(conditions)}" if conditions else ""

    with get_db() as conn:
        query = f"""
            SELECT
                signal_name,
                device_id,
                MIN(value) as min_val,
                MAX(value) as max_val,
                AVG(value) as avg_val,
                COUNT(*) as sample_count
            FROM telemetry
            {where}
            GROUP BY signal_name, device_id
        """
        rows = conn.execute(query, params).fetchall()
        return [dict(row) for row in rows]


def cleanup_old_data(retention_days: int = 30):
    """Delete telemetry data older than retention period."""
    cutoff = (datetime.utcnow() - timedelta(days=retention_days)).isoformat()
    with get_db() as conn:
        result = conn.execute("DELETE FROM telemetry WHERE timestamp < ?", (cutoff,))
        return result.rowcount


def store_alert(alert: Dict[str, Any]):
    """Store an alert event."""
    with get_db() as conn:
        conn.execute(
            """INSERT INTO alerts (rule_id, device_id, signal_name, value, threshold, condition, severity)
               VALUES (?, ?, ?, ?, ?, ?, ?)""",
            (alert.get('rule_id'), alert.get('device_id'), alert['signal_name'],
             alert['value'], alert['threshold'], alert['condition'], alert.get('severity', 'warning'))
        )


def get_alerts(limit: int = 100, severity: Optional[str] = None, unresolved_only: bool = False) -> List[Dict]:
    """Get alert history."""
    conditions = []
    params = []
    if severity:
        conditions.append("severity = ?")
        params.append(severity)
    if unresolved_only:
        conditions.append("resolved_at IS NULL")

    where = f"WHERE {' AND '.join(conditions)}" if conditions else ""
    with get_db() as conn:
        params.append(limit)
        rows = conn.execute(f"SELECT * FROM alerts {where} ORDER BY created_at DESC LIMIT ?", params).fetchall()
        return [dict(row) for row in rows]


# Initialize on import
init_db()