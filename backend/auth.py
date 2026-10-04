"""
Simple Token Authentication for theSpeaker (3 boards)

Provides optional API key authentication for securing endpoints.
Tokens are stored in a local JSON file for simplicity.

Usage:
  - Set ENABLE_AUTH=1 environment variable to activate
  - Generate tokens via: python -c "from auth import generate_token; print(generate_token('admin'))"
  - Pass token in header: Authorization: Bearer <token>
"""

import os
import json
import secrets
import hashlib
from pathlib import Path
from datetime import datetime
from typing import Optional
from functools import wraps

from fastapi import Request, HTTPException, Depends
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials

ENABLE_AUTH = os.environ.get('ENABLE_AUTH', '0') == '1'
TOKENS_FILE = Path(os.environ.get('TOKENS_FILE', 'data/tokens.json'))

security = HTTPBearer(auto_error=False)


def _load_tokens() -> dict:
    """Load tokens from file."""
    if not TOKENS_FILE.exists():
        return {}
    try:
        return json.loads(TOKENS_FILE.read_text())
    except (json.JSONDecodeError, IOError):
        return {}


def _save_tokens(tokens: dict):
    """Save tokens to file."""
    TOKENS_FILE.parent.mkdir(parents=True, exist_ok=True)
    TOKENS_FILE.write_text(json.dumps(tokens, indent=2))


def _hash_token(token: str) -> str:
    """Hash a token for safe storage."""
    return hashlib.sha256(token.encode()).hexdigest()


def generate_token(name: str = "default", role: str = "admin") -> str:
    """Generate a new API token."""
    token = secrets.token_urlsafe(32)
    tokens = _load_tokens()
    tokens[_hash_token(token)] = {
        "name": name,
        "role": role,
        "created_at": datetime.utcnow().isoformat(),
    }
    _save_tokens(tokens)
    return token


def validate_token(token: str) -> Optional[dict]:
    """Validate a token and return its metadata."""
    tokens = _load_tokens()
    hashed = _hash_token(token)
    return tokens.get(hashed)


async def get_current_user(
    credentials: Optional[HTTPAuthorizationCredentials] = Depends(security),
) -> Optional[dict]:
    """FastAPI dependency for authentication.
    
    Returns user info if auth is enabled and token is valid.
    Returns None if auth is disabled (always allows access).
    Raises HTTPException if auth is enabled but token is invalid.
    """
    if not ENABLE_AUTH:
        return None  # Auth disabled, allow all
    
    if not credentials:
        raise HTTPException(status_code=401, detail="Authentication required")
    
    user = validate_token(credentials.credentials)
    if not user:
        raise HTTPException(status_code=403, detail="Invalid or expired token")
    
    return user


def require_auth(func):
    """Decorator for routes that require authentication (when enabled)."""
    @wraps(func)
    async def wrapper(*args, request: Request = None, **kwargs):
        if ENABLE_AUTH:
            auth_header = request.headers.get("Authorization", "") if request else ""
            if not auth_header.startswith("Bearer "):
                raise HTTPException(status_code=401, detail="Authentication required")
            token = auth_header[7:]
            if not validate_token(token):
                raise HTTPException(status_code=403, detail="Invalid token")
        return await func(*args, request=request, **kwargs)
    return wrapper


# Generate a default token on first run if auth is enabled
if ENABLE_AUTH and not TOKENS_FILE.exists():
    default_token = generate_token("default-admin", "admin")
    print(f"🔐 Auth enabled. Default token: {default_token}")
    print(f"   Pass as: Authorization: Bearer {default_token}")