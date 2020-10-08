#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Unit test for uhd_images_downloader
"""

import unittest
import uhd_images_downloader

def mk_env(username, password):
    env = {}
    if username is not None:
        env[uhd_images_downloader._USERNAME_VARIABLE] = username
    if password is not None:
        env[uhd_images_downloader._PASSWORD_VARIABLE] = password
    return env

class PyImageDownloaderTest(unittest.TestCase):
    """ Test Python image downloader functionality """
    def test_parse_auth_works(self):
        self.assertEqual(
            uhd_images_downloader.parse_auth(mk_env("bbrother", "2+2=5")),
            ("bbrother", "2+2=5")
        )

    def test_parse_auth_empty_on_empty(self):
        self.assertEqual(
            uhd_images_downloader.parse_auth(mk_env(None, None)),
            None
        )
        self.assertEqual(
            uhd_images_downloader.parse_auth(mk_env("", None)),
            None
        )

    def test_throws_on_empty_pw(self):
        with self.assertRaises(RuntimeError):
            uhd_images_downloader.parse_auth(mk_env("username", None))

    def test_throws_on_empty_username(self):
        with self.assertRaises(RuntimeError):
            uhd_images_downloader.parse_auth(mk_env(None, "password"))
