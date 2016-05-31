#! /usr/bin/env python
# encoding: utf-8

APPNAME = 'noroi'
VERSION = '0.1'

top = '.'
out = 'build'

def options(ctx):
    ctx.load('compiler_cxx')
    ctx.add_option('--debug', action='store', default=False, help='Build with debug symbols')
    ctx.recurse('glad lib test')

def configure(ctx):
    ctx.load('compiler_cxx')
    if ctx.options.debug == 'true':
        ctx.env.append_value('CFLAGS', ['-g', '-Wall'])
    else:
        ctx.env.append_value('CFLAGS', ['-O3'])

    ctx.recurse('glad lib test')

def build(ctx):
    ctx.recurse('glad lib test')

def test(ctx):
    return ctx.exec_command('build/test/noroi_test')
