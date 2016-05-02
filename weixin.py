#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Filename:     weixind.py
# Author:       Liang Cha<ckmx945@gmail.com>
# CreateDate:   2014-05-15

import os
import web
import time
import types
import hashlib
import base64
#import memcache
#import RPi.GPIO as GPIO
#from lxml import etree
#from weixin import WeiXinClient
#from weixin import APIError
#from weixin import AccessTokenError


_TOKEN = 'idol'
_URLS = (
    '/weixin', 'weixinserver',
)


def _check_hash(data):
	
    signature = data.signature
    timestamp = data.timestamp
    nonce = data.nonce
    list = [_TOKEN, timestamp, nonce]
    list.sort()
    sha1 = hashlib.sha1()
    map(sha1.update, list)
    hashcode = sha1.hexdigest()
    msg = 'get, %s' % data
    
    if hashcode == signature: return True
    return False




    def GET(self):
        data = web.input()
        try:
            if _check_hash(data):
                return data.echostr
        except Exception, e:
            #print e
            return None

    


#application = web.application(_URLS, globals()).wsgifunc()
application = web.application(_URLS, globals())

if __name__ == "__main__":
    application.run()
