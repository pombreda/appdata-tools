#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Licensed under the GNU General Public License Version 2
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# Copyright (C) 2013
#    Richard Hughes <richard@hughsie.com>
#

import os
import sys
import csv
import textwrap

def quote(text):
    text = text.replace("&", "&amp;")
    text = text.replace("<", "&lt;")
    text = text.replace(">", "&gt;")
    text = text.replace("\"", "&#34;")
    text = text.replace("\'", "&#39;")
    return _to_utf8(text)

def main():

    got_valid = False
    homedir = os.getenv("HOME")
    csvfile = open(homedir + "/unloved.csv", 'r')

    spamreader = csv.reader(csvfile, delimiter=',', quotechar='\"')
    for data in spamreader:
        if data[1].startswith('desktop file name'):
            got_valid = True
            continue
        if not got_valid:
            continue
        if len(data) != 10:
            print data[1], 'INVALID', len(data)
            continue

        s = '<?xml version="1.0" encoding="UTF-8"?>\n'
        s = s + '<application>\n'
        s = s + '  <id type=\"desktop\">' + data[1] + '.desktop</id>\n'
        s = s + '  <metadata_license>CC0</metadata_license>\n'
        if len(data[2]) > 0:
            s = s + '  <name>' + data[2] + '</name>\n'
        if len(data[3]) > 0:
            s = s + '  <summary>' + data[3] + '</summary>\n'
        number_paras = 0
        if len(data[4]) > 0:
            s = s + '  <description>\n'
            for para in data[4].split('|'):
                number_paras = number_paras + 1
                s = s + '    <p>\n'
                para = para.replace('\n', ' ')
                para = para.replace('  ', ' ')
                para = para.replace('. ', '.\n')

                for sentance in para.split('\n'):
                    wrapped = textwrap.wrap(sentance, 74)
                    for split in wrapped:
                        s = s + '      ' + split + '\n'
                s = s + '    </p>\n'
            s = s + '  </description>\n'
        else:
            continue
        if number_paras < 2 or number_paras > 3:
            continue
        if len(data[5]) > 0:
            s = s + '  <url type="homepage">' + data[5] + '</url>\n'
        if len(data[6]) > 0:
            s = s + '  <screenshots>\n'
            screenshots = data[6].replace('\n', '').split('|')
            i = 0
            for screenshot in screenshots:
                if i == 0:
                    s = s + '    <screenshot type="default">' + screenshot + '</screenshot>\n'
                else:
                    s = s + '    <screenshot>' + screenshot + '</screenshot>\n'
                i = i + 1
            s = s + '  </screenshots>\n'
        else:
            continue
        if len(data[7]) > 0:
            s = s + '  <updatecontact>' + data[7] + '</updatecontact>\n'
        else:
            s = s + '  <!-- FIXME: change this to an upstream email address for spec updates\n'
            s = s + '  <updatecontact>someone_who_cares@upstream_project.org</updatecontact>\n'
            s = s + '   -->\n'
        if len(data[8]) > 0:
            s = s + '  <project_group>' + data[8] + '</project_group>\n'
        s = s + '</application>\n'

        filename = homedir + '/' + data[1] + '.appdata.xml'
        outfile = open(filename, 'w')
        outfile.write(s)
        outfile.close()
        print 'writing', filename

    sys.exit(0)

if __name__ == "__main__":
    main()
