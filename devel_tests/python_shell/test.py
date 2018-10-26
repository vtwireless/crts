#!/usr/bin/env python

import os, sys, inspect



importDir = os.path.abspath(os.path.join(os.path.dirname(__file__),'..','..','lib','python2.7'))

print importDir

sys.path.append(importDir)

import shell

sh = shell.Shell("foo")

sh.help()
