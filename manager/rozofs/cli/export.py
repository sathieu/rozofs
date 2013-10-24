# -*- coding: utf-8 -*-
#
# Copyright (c) 2013 Fizians SAS. <http://www.fizians.com>
# This file is part of Rozofs.
#
# Rozofs is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 2.
#
# Rozofs is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see
# <http://www.gnu.org/licenses/>.

import sys
from rozofs.core.platform import Platform, Role
from rozofs.core.agent import ServiceStatus
from rozofs.cli.output import ordered_puts
from rozofs.cli.output import puts
from collections import OrderedDict


def list(platform, args):
    configurations = platform.get_configurations([args.exportd], Role.EXPORTD)
    if configurations[args.exportd] is None:
        raise Exception("exportd node is off line.")

    configuration = configurations[args.exportd][Role.EXPORTD]

    for eid, econfig in configuration.exports.items():
        exp_l = {}
        exp_l['eid ' + str(eid)] = OrderedDict([
            ('vid', econfig.vid),
            ('root', econfig.root),
            ('md5', econfig.md5),
            ('squota', econfig.squota),
            ('hquota', econfig.hquota)
        ])
        ordered_puts(exp_l)

def create(platform, args):
    platform.create_export(args.vid[0], args.name, args.passwd, args.squota, args.hquota)


def update(platform, args):
    platform.update_export(args.eid[0], args.current, args.passwd, args.squota, args.hquota)


def remove(platform, args):
    if not args.eids:
        args.eids = None

    platform.remove_export(args.eids, args.force)

def get(platform, args):
    configurations = platform.get_configurations([args.exportd], Role.EXPORTD)
    if configurations[args.exportd] is None:
        raise Exception("exportd node is off line.")

    configuration = configurations[args.exportd][Role.EXPORTD]

    for eid, estat in configuration.stats.estats.items():
        exp_l = {}
        exp_l['eid ' + str(eid)] = OrderedDict([
            ('bsize', estat.bsize),
            ('blocks', estat.blocks),
            ('bfree', estat.bfree),
            ('files', estat.files),
            ('ffree', estat.ffree)
        ])
        ordered_puts(exp_l)

def mount(platform, args):
    if not args.eids:
        args.eids = None

    platform.mount_export(args.eids, args.nodes)


def umount(platform, args):
    if not args.eids:
        args.eids = None

    platform.umount_export(args.eids, args.nodes)


def dispatch(args):
    p = Platform(args.exportd)
    globals()[args.action.replace('-', '_')](p, args)