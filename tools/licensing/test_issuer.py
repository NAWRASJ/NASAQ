# SPDX-License-Identifier: GPL-3.0-or-later
import unittest
from datetime import datetime, timezone
from unittest.mock import patch
import nasaq_licenses as n

class IssuerTests(unittest.TestCase):
    def test_random_keys(self):
        keys = {n.new_key() for _ in range(2000)}
        self.assertEqual(len(keys), 2000)
        for key in keys:
            self.assertEqual(len(key), 16)
            self.assertTrue(set(key) <= set(n.ALPHABET))
    def test_duration_and_devices(self):
        doc=n.issue_payload('policy',30,3,key='TEST',now=datetime(2026,1,1,tzinfo=timezone.utc))
        a=doc['data']['attributes']
        self.assertEqual(a['maxMachines'],3)
        self.assertEqual(a['expiry'],'2026-01-31T00:00:00+00:00')
    def test_invalid_limits(self):
        for days,devices in ((0,1),(30,0),(-1,2),(36501,1)):
            with self.assertRaises(ValueError): n.issue_payload('policy',days,devices)
    def test_policy_checks(self):
        doc={'data':{'attributes':dict(n.POLICY),'relationships':{'product':{'data':{'id':'product'}}}}}
        n.verify_policy(doc,'product')
        doc['data']['attributes']['overageStrategy']='ALWAYS_ALLOW_OVERAGE'
        with self.assertRaises(ValueError):n.verify_policy(doc,'product')
    def test_wrong_product(self):
        with self.assertRaises(ValueError):n.verify_policy({'data':{}},'product')
    def test_identifier_rejects_url(self):
        for s in ('../other','id?token=secret','https://other',''):
            with self.assertRaises(ValueError): n.identifier(s)
    def test_missing_token(self):
        with self.assertRaises(ValueError): n.Api('account','').request('POST','/licenses',{})
    def test_no_redirect(self):
        self.assertIsNone(n.NoRedirect().redirect_request(None,None,None,None,None,None))
    def test_transport_payload_and_secret_not_url(self):
        class Response:
            def __enter__(self):return self
            def __exit__(self,*args):pass
            def read(self,n):return b'{"data":{"id":"created"}}'
        class Opener:
            def open(self,req,timeout):
                self.request=req;return Response()
        opener=Opener(); api=n.Api('account','test-only-token',opener)
        self.assertEqual(api.request('POST','/licenses',{'test':1})['data']['id'],'created')
        self.assertNotIn('test-only-token',opener.request.full_url)
        self.assertEqual(opener.request.get_header('Authorization'),'Bearer test-only-token')

if __name__=='__main__':unittest.main(verbosity=2)
