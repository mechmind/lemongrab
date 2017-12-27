lemongrab
=========

Simple XMPP MUC bot, serving one conference at a time, written in C++.

This is a pet project, not intended for general use.

[![Build Status](https://travis-ci.org/Chemrat/lemongrab.svg?branch=master)](https://travis-ci.org/Chemrat/lemongrab)
[![Coverage Status](https://coveralls.io/repos/github/Chemrat/lemongrab/badge.svg?branch=master)](https://coveralls.io/github/Chemrat/lemongrab?branch=master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/49f2e50d3d81475d9bb0915b81368494)](https://www.codacy.com/app/jazzvoid/lemongrab?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Chemrat/lemongrab&amp;utm_campaign=Badge_Grade)
[![No Maintenance Intended](http://unmaintained.tech/badge.svg)](http://unmaintained.tech/)



List of features:

* URL preview - posts page title for URLs posted by participants + url history
* Dice roller
* Last seen - check when specific user last time visited the conference (bot must see user JIDs for this to work)
* Pager - leave a public message to a currently absent user
* RSS - poll news feeds and dump latest items in chat
* Github Webhooks - notify chat about GitHub events (issues, stars, forks & pull requests)
* Leauge of Legends lookup - see if player is currently in game
* Quotes database
* Simple polls
* Discord bridge

How to build/run
================

Dependencies:

* gloox
* curl
* libevent
* jsoncpp
* boost_locale
* glog
* pugixml
* sqlite, sqlite_orm
* hexicord
* gtest (if you're building tests)

Build instructions:

```
mkdir build
cd build
cmake ..
make
```

* Rename config.toml.default to config.toml, edit according to your needs (config file should be self-explanatory) and put it in the work directory
* Create database folder set by General.DBPathPrefix (defaults to /var/db/lemongrab)
* Create logs folder set by General.LogPathPrefix (defaults to /var/log/lemongrab)

Commands
========
General commands: !getversion, !help, !die (asks bot to exit)

Use !help %module_name% to get commands, specific to a module

Extending
=========
Implement `LemonHandler` interface (see `handlers/lemonhandler.h`, see `handlers/goodenough.*` for example)

Register handler in `Bot::Run()` in `bot.cpp`
