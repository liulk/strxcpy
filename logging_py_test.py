#!/usr/bin/env python

import logging

def main():
  format = ''
  for v in open('logging_variables.txt'):
    if v.strip() == 'funcName': continue
    format += '%(' + v.strip() + ')s '

  logging.basicConfig(level=logging.INFO, format=format)

  for i in xrange(0, 5):
    logging.info('i = %d' % i)

if __name__ == '__main__':
  main()

