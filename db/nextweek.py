#!/usr/bin/python

import time
import datetime

day = (24*60)*60*7
tomorrow = datetime.datetime.fromtimestamp(time.time() + day)
print tomorrow.strftime("%a, %d %b %Y")
