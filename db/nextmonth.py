#!/usr/bin/python

import time
import datetime
from dateutil.relativedelta import relativedelta

tmp = datetime.datetime.today() + relativedelta(months=1)
nextmonth = datetime.datetime(tmp.year, tmp.month, 1)
print nextmonth.strftime("%a, %d %b %Y")
