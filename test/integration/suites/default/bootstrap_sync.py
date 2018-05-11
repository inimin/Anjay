# -*- coding: utf-8 -*-
#
# Copyright 2017-2018 AVSystem <avsystem@avsystem.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import re

from framework.lwm2m_test import *


class ClientIgnoresNonBootstrapTrafficDuringBootstrap(test_suite.Lwm2mSingleServerTest):
    PSK_IDENTITY = b'test-identity'
    PSK_KEY = b'test-key'

    def setUp(self):
        self.setup_demo_with_servers(servers=[Lwm2mServer(coap.DtlsServer(psk_key=self.PSK_KEY, psk_identity=self.PSK_IDENTITY))],
                                     bootstrap_server=Lwm2mServer(coap.DtlsServer(psk_key=self.PSK_KEY, psk_identity=self.PSK_IDENTITY)),
                                     extra_cmdline_args=['--identity',
                                                         str(binascii.hexlify(self.PSK_IDENTITY), 'ascii'),
                                                         '--key', str(binascii.hexlify(self.PSK_KEY), 'ascii')],
                                     auto_register=False)

        self.bootstrap_server.listen()
        self.assertDemoRegisters(self.serv)

    def runTest(self):
        req = Lwm2mCreate('/1337')
        self.serv.send(req)
        res = self.serv.recv()
        self.assertMsgEqual(Lwm2mCreated.matching(req)(), res)
        self.assertEqual('/1337/1', res.get_location_path())

        # force an Update so that change to the data model does not get notified later
        self.communicate('send-update')
        self.assertDemoUpdatesRegistration(content=ANY)

        req = Lwm2mRead('/1337/1/1')
        self.serv.send(req)
        res = self.serv.recv()
        self.assertMsgEqual(Lwm2mContent.matching(req)(), res)
        self.assertEqual(b'0', res.content)

        # "spurious" Server Initiated Bootstrap
        req = Lwm2mWrite('/1337/1/1', b'42')
        self.bootstrap_server.send(req)
        self.assertMsgEqual(Lwm2mChanged.matching(req)(), self.bootstrap_server.recv())

        # now regular server shall not be able to communicate with the client
        req = Lwm2mExecute('/1337/1/2')
        self.serv.send(req)
        with self.assertRaises(OSError):
            self.serv.recv(timeout_s=5)

        self.assertEqual(1, self.get_socket_count())

        # Bootstrap Finish
        self.bootstrap_server.send(Lwm2mBootstrapFinish())
        self.assertIsInstance(self.bootstrap_server.recv(), Lwm2mChanged)

        # client reconnects with DTLS session resumption
        self.assertDtlsReconnect()

        # now we shall be able to do that Execute
        req = Lwm2mExecute('/1337/1/2')
        self.serv.send(req)
        res = self.serv.recv()
        self.assertMsgEqual(Lwm2mChanged.matching(req)(), res)

        # verify that Execute was performed just once
        req = Lwm2mRead('/1337/1/1')
        self.serv.send(req)
        res = self.serv.recv()
        self.assertMsgEqual(Lwm2mContent.matching(req)(), res)
        self.assertEqual(b'43', res.content)
