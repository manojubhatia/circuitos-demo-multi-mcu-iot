#!/bin/bash
# Setup Observability Stack for theSpeaker (3 boards)
# Optional: Sets up InfluxDB (time-series database) and Grafana (visualization)
#
# Usage:
#   ./setup_observability.sh           # Install and setup InfluxDB + Grafana
#   ./setup_observability.sh --influx  # Install only InfluxDB
#   ./setup_observability.sh --grafana # Install only Grafana
#   ./setup_observability.sh --stop    # Stop all observability services
#   ./setup_observability.sh --status  # Check service status

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DATA_DIR="$PROJECT_ROOT/data"

GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m'

success() { echo -e "${GREEN}✓ $1${NC}"; }
info() { echo -e "${CYAN}ℹ $1${NC}"; }
warn() { echo -e "${YELLOW}⚠ $1${NC}"; }
error() { echo -e "${RED}✗ $1${NC}"; exit 1; }

# Configuration
INFLUXDB_PORT=8086
GRAFANA_PORT=3000
INFLUXDB_ORG="thespeaker (3 boards)"
INFLUXDB_BUCKET="thespeaker (3 boards)_telemetry"
INFLUXDB_USER="admin"
INFLUXDB_PASSWORD="adminpassword"
INFLUXDB_TOKEN=""

# Check for Docker
check_docker() {
    if ! command -v docker &> /dev/null; then
        error "Docker is not installed. Please install Docker first: https://docs.docker.com/get-docker/"
    fi
    if ! docker info &> /dev/null; then
        error "Docker daemon is not running. Please start Docker."
    fi
    success "Docker is available"
}

# Create data directories
setup_dirs() {
    mkdir -p "$DATA_DIR/influxdb"
    mkdir -p "$DATA_DIR/grafana"
    chmod -R 777 "$DATA_DIR/grafana"  # Grafana needs write access
    success "Data directories created"
}

# Setup InfluxDB
setup_influxdb() {
    info "Setting up InfluxDB..."
    
    # Check if already running
    if docker ps | grep -q influxdb_thespeaker (3 boards); then
        warn "InfluxDB is already running"
        return
    fi
    
    # Remove old container if exists
    docker rm -f influxdb_thespeaker (3 boards) 2>/dev/null || true
    
    # Start InfluxDB
    docker run -d \
        --name influxdb_thespeaker (3 boards) \
        -p $INFLUXDB_PORT:8086 \
        -v "$DATA_DIR/influxdb:/var/lib/influxdb2" \
        -e DOCKER_INFLUXDB_INIT_MODE=setup \
        -e DOCKER_INFLUXDB_INIT_USERNAME=$INFLUXDB_USER \
        -e DOCKER_INFLUXDB_INIT_PASSWORD=$INFLUXDB_PASSWORD \
        -e DOCKER_INFLUXDB_INIT_ORG=$INFLUXDB_ORG \
        -e DOCKER_INFLUXDB_INIT_BUCKET=$INFLUXDB_BUCKET \
        influxdb:2.7
    
    # Wait for InfluxDB to be ready
    info "Waiting for InfluxDB to start..."
    for i in {1..30}; do
        if curl -s "http://localhost:$INFLUXDB_PORT/health" | grep -q "pass"; then
            break
        fi
        sleep 1
    done
    
    # Get the token
    INFLUXDB_TOKEN=$(docker exec influxdb_thespeaker (3 boards) influx auth list --json 2>/dev/null | grep -o '"token":"[^"]*"' | head -1 | cut -d'"' -f4)
    
    if [ -n "$INFLUXDB_TOKEN" ]; then
        # Save token to config file
        echo "INFLUXDB_URL=http://localhost:$INFLUXDB_PORT" > "$PROJECT_ROOT/.env.observability"
        echo "INFLUXDB_TOKEN=$INFLUXDB_TOKEN" >> "$PROJECT_ROOT/.env.observability"
        echo "INFLUXDB_ORG=$INFLUXDB_ORG" >> "$PROJECT_ROOT/.env.observability"
        echo "INFLUXDB_BUCKET=$INFLUXDB_BUCKET" >> "$PROJECT_ROOT/.env.observability"
        success "InfluxDB started and token saved to .env.observability"
    else
        warn "Could not retrieve InfluxDB token. You may need to configure it manually."
    fi
    
    success "InfluxDB is running at http://localhost:$INFLUXDB_PORT"
    info "  - Organization: $INFLUXDB_ORG"
    info "  - Bucket: $INFLUXDB_BUCKET"
    info "  - Username: $INFLUXDB_USER"
}

# Setup Grafana
setup_grafana() {
    info "Setting up Grafana..."
    
    # Check if already running
    if docker ps | grep -q grafana_thespeaker (3 boards); then
        warn "Grafana is already running"
        return
    fi
    
    # Remove old container if exists
    docker rm -f grafana_thespeaker (3 boards) 2>/dev/null || true
    
    # Create Grafana datasource provisioning
    mkdir -p "$DATA_DIR/grafana/provisioning/datasources"
    mkdir -p "$DATA_DIR/grafana/provisioning/dashboards"
    
    # Load InfluxDB token if available
    if [ -f "$PROJECT_ROOT/.env.observability" ]; then
        source "$PROJECT_ROOT/.env.observability"
    fi
    
    # Create InfluxDB datasource config
    cat > "$DATA_DIR/grafana/provisioning/datasources/influxdb.yaml" << DATASOURCE_EOF
apiVersion: 1
datasources:
  - name: InfluxDB
    type: influxdb
    access: proxy
    url: http://host.docker.internal:$INFLUXDB_PORT
    jsonData:
      version: Flux
      organization: $INFLUXDB_ORG
      defaultBucket: $INFLUXDB_BUCKET
    secureJsonData:
      token: ${INFLUXDB_TOKEN:-your-token-here}
DATASOURCE_EOF
    
    # Create dashboard provisioning config
    cat > "$DATA_DIR/grafana/provisioning/dashboards/default.yaml" << DASH_PROV_EOF
apiVersion: 1
providers:
  - name: 'default'
    orgId: 1
    folder: ''
    type: file
    disableDeletion: false
    updateIntervalSeconds: 10
    options:
      path: /var/lib/grafana/dashboards
DASH_PROV_EOF
    
    # Create sample dashboard
    mkdir -p "$DATA_DIR/grafana/dashboards"
    cat > "$DATA_DIR/grafana/dashboards/thespeaker (3 boards)_dashboard.json" << DASH_EOF
{
  "annotations": {
    "list": []
  },
  "editable": true,
  "fiscalYearStartMonth": 0,
  "graphTooltip": 0,
  "id": null,
  "links": [],
  "liveNow": true,
  "panels": [
    {
      "datasource": {
        "type": "influxdb",
        "uid": "influxdb"
      },
      "fieldConfig": {
        "defaults": {
          "color": {
            "mode": "palette-classic"
          },
          "custom": {
            "axisBorderShow": false,
            "axisCenteredZero": false,
            "axisColorMode": "text",
            "axisLabel": "",
            "axisPlacement": "auto",
            "barAlignment": 0,
            "drawStyle": "line",
            "fillOpacity": 10,
            "gradientMode": "none",
            "hideFrom": {
              "legend": false,
              "tooltip": false,
              "viz": false
            },
            "lineInterpolation": "smooth",
            "lineWidth": 2,
            "pointSize": 5,
            "scaleDistribution": {
              "type": "linear"
            },
            "showPoints": "never",
            "spanNulls": false,
            "stacking": {
              "group": "A",
              "mode": "none"
            },
            "thresholdsStyle": {
              "mode": "off"
            }
          },
          "mappings": [],
          "thresholds": {
            "mode": "absolute",
            "steps": [
              {
                "color": "green",
                "value": null
              }
            ]
          },
          "unit": "short"
        },
        "overrides": []
      },
      "gridPos": {
        "h": 8,
        "w": 24,
        "x": 0,
        "y": 0
      },
      "id": 1,
      "options": {
        "legend": {
          "calcs": ["mean", "lastNotNull", "max", "min"],
          "displayMode": "table",
          "placement": "bottom",
          "showLegend": true
        },
        "tooltip": {
          "mode": "multi",
          "sort": "none"
        }
      },
      "targets": [
        {
          "datasource": {
            "type": "influxdb",
            "uid": "influxdb"
          },
          "query": "from(bucket: \"thespeaker (3 boards)_telemetry\")\n  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)\n  |> filter(fn: (r) => r[\"_measurement\"] == \"sensor_data\")\n  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)\n  |> yield(name: \"mean\")",
          "refId": "A"
        }
      ],
      "title": "Sensor Data - theSpeaker (3 boards)",
      "type": "timeseries"
    }
  ],
  "refresh": "5s",
  "schemaVersion": 38,
  "style": "dark",
  "tags": ["thespeaker (3 boards)", "telemetry"],
  "templating": {
    "list": []
  },
  "time": {
    "from": "now-5m",
    "to": "now"
  },
  "timepicker": {},
  "timezone": "",
  "title": "theSpeaker (3 boards) Telemetry Dashboard",
  "uid": "thespeaker (3 boards)_dashboard",
  "version": 1
}
DASH_EOF
    
    # Start Grafana
    docker run -d \
        --name grafana_thespeaker (3 boards) \
        -p $GRAFANA_PORT:3000 \
        --add-host=host.docker.internal:host-gateway \
        -v "$DATA_DIR/grafana:/var/lib/grafana" \
        -v "$DATA_DIR/grafana/provisioning:/etc/grafana/provisioning" \
        -v "$DATA_DIR/grafana/dashboards:/var/lib/grafana/dashboards" \
        -e GF_SECURITY_ADMIN_USER=admin \
        -e GF_SECURITY_ADMIN_PASSWORD=admin \
        -e GF_USERS_ALLOW_SIGN_UP=false \
        grafana/grafana:latest
    
    # Wait for Grafana to be ready
    info "Waiting for Grafana to start..."
    for i in {1..30}; do
        if curl -s "http://localhost:$GRAFANA_PORT/api/health" | grep -q "ok"; then
            break
        fi
        sleep 1
    done
    
    success "Grafana is running at http://localhost:$GRAFANA_PORT"
    info "  - Username: admin"
    info "  - Password: admin"
    info "  - Dashboard: theSpeaker (3 boards) Telemetry Dashboard"
}

# Stop services
stop_services() {
    info "Stopping observability services..."
    docker stop influxdb_thespeaker (3 boards) 2>/dev/null && success "InfluxDB stopped" || true
    docker stop grafana_thespeaker (3 boards) 2>/dev/null && success "Grafana stopped" || true
}

# Check status
check_status() {
    echo ""
    echo "=== Observability Stack Status ==="
    echo ""
    
    if docker ps | grep -q influxdb_thespeaker (3 boards); then
        success "InfluxDB: Running at http://localhost:$INFLUXDB_PORT"
    else
        warn "InfluxDB: Not running"
    fi
    
    if docker ps | grep -q grafana_thespeaker (3 boards); then
        success "Grafana: Running at http://localhost:$GRAFANA_PORT"
    else
        warn "Grafana: Not running"
    fi
    
    echo ""
}

# Print usage info
print_usage_info() {
    echo ""
    echo "=============================================="
    echo " Observability Stack Ready!"
    echo "=============================================="
    echo ""
    info "InfluxDB: http://localhost:$INFLUXDB_PORT"
    info "Grafana:  http://localhost:$GRAFANA_PORT"
    echo ""
    info "To send data to InfluxDB from your backend, add this to your code:"
    echo ""
    echo "  from influxdb_client import InfluxDBClient, Point"
    echo "  from influxdb_client.client.write_api import SYNCHRONOUS"
    echo ""
    echo "  # Load config from .env.observability"
    echo "  client = InfluxDBClient(url='http://localhost:$INFLUXDB_PORT', token='YOUR_TOKEN', org='$INFLUXDB_ORG')"
    echo "  write_api = client.write_api(write_options=SYNCHRONOUS)"
    echo ""
    echo "  # Write data"
    echo "  point = Point('sensor_data').tag('device', 'esp32').field('temperature', 25.5)"
    echo "  write_api.write(bucket='$INFLUXDB_BUCKET', record=point)"
    echo ""
}

# Main
main() {
    echo "=============================================="
    echo " theSpeaker (3 boards) Observability Setup"
    echo "=============================================="
    echo ""
    
    case "${1:-}" in
        --influx)
            check_docker
            setup_dirs
            setup_influxdb
            ;;
        --grafana)
            check_docker
            setup_dirs
            setup_grafana
            ;;
        --stop)
            stop_services
            ;;
        --status)
            check_status
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  (none)     Install and setup InfluxDB + Grafana"
            echo "  --influx   Install only InfluxDB"
            echo "  --grafana  Install only Grafana"
            echo "  --stop     Stop all observability services"
            echo "  --status   Check service status"
            echo "  --help     Show this help message"
            ;;
        *)
            check_docker
            setup_dirs
            setup_influxdb
            setup_grafana
            print_usage_info
            ;;
    esac
}

main "$@"
