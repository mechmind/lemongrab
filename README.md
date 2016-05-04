lemongrab
=========

Simple XMPP MUC bot, serving one conference at a time, written in C++

[![Build Status](https://travis-ci.org/Chemrat/lemongrab.svg?branch=master)](https://travis-ci.org/Chemrat/lemongrab)
[![Coverage Status](https://coveralls.io/repos/github/Chemrat/lemongrab/badge.svg?branch=master)](https://coveralls.io/github/Chemrat/lemongrab?branch=master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/8766/badge.svg)](https://scan.coverity.com/projects/8766)

List of features:

* URL preview - posts page title for URLs posted by participants + url history
* Dice roller
* Last seen - check when specific user last time visited the conference (bot must see user JIDs for this to work)
* Pager - leave a public message to a currently absent user
* Github Webhooks - notify chat about GitHub events (issues, stars, forks & pull requests)
* TeamSpeak monitor - notify chat when someone connects or disconnects from TeamSpeak server (needs ServerQuery account) and transport messages from/to conference
* Leauge of Legends lookup - see if player is currently in game

How to build/run
================

Dependencies:

* gloox
* curl
* leveldb
* libevent
* jsoncpp
* boost_locale
* glog
* gtest (if you're building tests)

Build instructions:

```
mkdir build
cd build
cmake ..
make
```

Rename config.ini.default to config.ini, edit according to your needs (config file should be self-explanatory) and put it in the work directory
Create a `logs` folder in work directory
Create a `db` folder in work directory and run bot executable: ./lemongrab

Commands
========
General commands: !getversion, !help, !die (asks bot to exit)

Use !help %module_name% to get commands, specific to a module

Extending
=========
Implement `LemonHandler` interface (see `handlers/lemonhandler.h`, see `handlers/goodenough.*` for example)

Register handler in `Bot::Run()` in `bot.cpp`
