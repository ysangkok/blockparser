#!/usr/bin/python

import time
import datetime
from dateutil.relativedelta import relativedelta

nextmonth = datetime.datetime.today() + relativedelta(months=1)
print nextmonth.strftime("%a, %d %b %Y")
