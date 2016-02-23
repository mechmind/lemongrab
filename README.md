lemongrab
=========

Simple XMPP MUC bot, written in C++, made for private use

[![Build Status](https://travis-ci.org/Chemrat/lemongrab.svg?branch=master)](https://travis-ci.org/Chemrat/lemongrab)

List of features:

* URL preview - posts page title for URLs posted by participants
* Dice roller
* Last seen - check when specific user last time visited the conference (bot must see user JIDs for this to work)
* Pager - leave a public message to a currently absent user
* Github Webhooks - notify chat about GitHub events

Dependencies:

* gloox
* curl
* leveldb
* libevent
* jsoncpp
* gtest
