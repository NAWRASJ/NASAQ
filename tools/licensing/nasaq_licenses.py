# SPDX-License-Identifier: GPL-3.0-or-later
"""Publisher-only Keygen administrator. Standard Python 3; no customer deployment."""
import argparse
from datetime import datetime, timedelta, timezone
import json
import os
import re
import secrets
import urllib.request
import urllib.error

ALPHABET = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789'
POLICY = dict(name='NASAQ commercial activation', strict=True, floating=True,
              scheme=None, maxMachines=1, requireFingerprintScope=True,
              requireProductScope=True, requireHeartbeat=False, requireCheckIn=False,
              authenticationStrategy='LICENSE', expirationStrategy='RESTRICT_ACCESS',
              expirationBasis='FROM_CREATION', overageStrategy='NO_OVERAGE')

def identifier(value):
    if not re.fullmatch(r'[A-Za-z0-9-]+', value or ''):
        raise ValueError('A valid public account/product/policy/resource ID is required')
    return value

def new_key():
    return ''.join(secrets.choice(ALPHABET) for _ in range(16))

def issue_payload(policy, days, devices, key=None, now=None):
    if not 1 <= days <= 36500 or not 1 <= devices <= 10000:
        raise ValueError('Days must be 1..36500 and devices 1..10000')
    now = now or datetime.now(timezone.utc)
    return {'data': {'type': 'licenses', 'attributes': {
        'key': key or new_key(), 'expiry': (now + timedelta(days=days)).isoformat(),
        'maxMachines': devices}, 'relationships': {
            'policy': {'data': {'type': 'policies', 'id': identifier(policy)}}}}}

class Api:
    def __init__(self, account, token, opener=None):
        self.base = 'https://api.keygen.sh/v1/accounts/' + identifier(account)
        self.token = token
        self.opener = opener or urllib.request.build_opener(NoRedirect)
    def request(self, method, path, payload=None):
        if not self.token:
            raise ValueError('Set KEYGEN_ADMIN_TOKEN locally, never in source or chat')
        request = urllib.request.Request(self.base + path,
            data=None if payload is None else json.dumps(payload).encode(), method=method,
            headers={'Authorization': 'Bearer ' + self.token,
                     'Content-Type': 'application/vnd.api+json', 'Accept': 'application/vnd.api+json'})
        try:
            with self.opener.open(request, timeout=20) as response:
                raw = response.read(1048577)
                if len(raw) > 1048576: raise ValueError('Oversized API response')
                return json.loads(raw) if raw else {}
        except urllib.error.HTTPError as exc:
            # Never print an error body, URL, authorization header or licence key.
            raise ValueError(f'Keygen returned HTTP {exc.code}; inspect dashboard. No automatic retry.') from None
        except (urllib.error.URLError, TimeoutError):
            raise ValueError('Connection failed; outcome may be unknown. Check dashboard before retrying.') from None

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs): return None

def verify_policy(document, product):
    data = document.get('data', {})
    if data.get('relationships', {}).get('product', {}).get('data', {}).get('id') != product:
        raise ValueError('Policy belongs to another product')
    attrs = data.get('attributes', {})
    for field in ('strict', 'floating', 'scheme', 'requireFingerprintScope', 'requireProductScope',
                  'requireHeartbeat', 'requireCheckIn', 'authenticationStrategy', 'expirationStrategy', 'overageStrategy'):
        if field not in attrs or attrs[field] != POLICY[field]:
            raise ValueError('Unsafe/incompatible policy field: ' + field)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--apply', action='store_true', help='Write to Keygen; default is a redacted dry run')
    sub = p.add_subparsers(dest='command', required=True)
    sub.add_parser('create-policy')
    issue = sub.add_parser('issue');issue.add_argument('--days', type=int, required=True);issue.add_argument('--devices', type=int, required=True)
    renew = sub.add_parser('renew');renew.add_argument('--id', required=True);renew.add_argument('--days', type=int, required=True)
    devices = sub.add_parser('set-devices');devices.add_argument('--id', required=True);devices.add_argument('--devices', type=int, required=True)
    for name in ('suspend', 'reinstate', 'list-devices', 'release-device'):
        sub.add_parser(name).add_argument('--id', required=True)
    a = p.parse_args()
    product = identifier(os.environ.get('KEYGEN_PRODUCT_ID', ''))
    api = Api(os.environ.get('KEYGEN_ACCOUNT_ID', ''), os.environ.get('KEYGEN_ADMIN_TOKEN', ''))
    method, payload = 'POST', None
    if a.command == 'create-policy':
        path = '/policies';payload = {'data': {'type': 'policies', 'attributes': POLICY,
            'relationships': {'product': {'data': {'type': 'products', 'id': product}}}}}
    elif a.command == 'issue':
        policy = identifier(os.environ.get('KEYGEN_POLICY_ID', ''))
        path = '/licenses';payload = issue_payload(policy, a.days, a.devices, key=None if a.apply else '<generated only on apply>')
        if a.apply: verify_policy(api.request('GET', '/policies/' + policy), product)
    else:
        rid = identifier(a.id)
        if a.command == 'release-device': method, path = 'DELETE', '/machines/' + rid
        elif a.command == 'list-devices': method, path = 'GET', '/licenses/' + rid + '/machines'
        elif a.command in ('suspend', 'reinstate'): path = '/licenses/' + rid + '/actions/' + a.command
        else:
            method, path = 'PATCH', '/licenses/' + rid
            if a.command == 'set-devices':
                if not 1 <= a.devices <= 10000: raise ValueError('Devices must be 1..10000')
                attrs = {'maxMachines': a.devices}
            else:
                if not 1 <= a.days <= 36500: raise ValueError('Days must be 1..36500')
                # An explicit new term from now, not a hidden change of existing expiry.
                attrs = {'expiry': (datetime.now(timezone.utc)+timedelta(days=a.days)).isoformat()}
            payload = {'data': {'type': 'licenses', 'id': rid, 'attributes': attrs}}
    if not a.apply:
        print(json.dumps({'dryRun': True, 'method': method, 'path': path, 'body': payload}, indent=2));return
    result = api.request(method, path, payload)
    data = result.get('data', {})
    if a.command == 'issue':
        expected = payload['data']['attributes']
        actual = data.get('attributes', {})
        if actual.get('key') != expected['key'] or actual.get('maxMachines') != a.devices:
            raise ValueError('Created licence differs from request; inspect dashboard before delivery')
        print(json.dumps({'id': data['id'], 'key': '-'.join(actual['key'][i:i+4] for i in range(0,16,4)),
                          'expiry': actual['expiry'], 'devices': actual['maxMachines']}, indent=2))
    elif a.command == 'list-devices':
        print(json.dumps({'devices': [{'id': d['id'], 'name': d.get('attributes', {}).get('name')} for d in data],
                          'nextPage': result.get('links', {}).get('next')}, indent=2))
    else: print(json.dumps({'completed': a.command, 'id': data.get('id') if isinstance(data, dict) else None}))

if __name__ == '__main__':
    try: main()
    except (ValueError, KeyError) as exc:
        raise SystemExit(str(exc))
