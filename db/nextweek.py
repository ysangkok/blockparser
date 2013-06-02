#!/usr/bin/python

import time
import datetime
from dateutil.relativedelta import relativedelta

# Next Saturday
today = datetime.datetime.today()
weekday = today.weekday()
addition = 5 - weekday
if addition <= 0:
  addition += 7
nextweek = today + relativedelta(days=addition)
print nextweek.strftime("%a, %d %b %Y")
