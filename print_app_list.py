#!/usr/bin/env python3
import os.path as path
import argparse
import sys

"""This script is used to generate the list of currently included
O_C original apps and Hemispher apps
"""

OC_APPS_PATH = 'OC_apps.cpp'
HEM_CONFIG_PATH = 'hemisphere_config.h'

def main(argv):
    args = parse_args(argv)

    HOME_PATH = args.src

    oc_apps = []
    oc_file = open(path.join(HOME_PATH, OC_APPS_PATH), 'r')
    for line in oc_file:
        line = line.strip()
        if line.find('DECLARE_APP') >= 0 and line[0] != '/' and line.find('#define') < 0:
            oc_apps.append(line.split(',')[2].strip().replace('"', ''))
    oc_file.close()

    hem_apps = []
    hem_file = open(path.join(HOME_PATH, HEM_CONFIG_PATH), 'r')
    for line in hem_file:
        line = line.strip()
        if line.find('DECLARE_APP') >= 0 and line[0] != '/' and line.find('#define') < 0:
            hem_apps.append(line.split(',')[2].strip().replace(')', ''))
    hem_file.close()

    oc_apps.sort()
    hem_apps.sort()

    print_md_formatted(oc_apps, hem_apps)

def print_md_formatted(oc_apps, hem_apps):
    print('## o_C APPS')
    print('')
    for app in oc_apps:
        print('- %s' % app)
    print('')

    print('## Hemisphere APPS')
    print('')
    for app in hem_apps:
        print('- %s' % app)
    print('')

def parse_args(argv):
    """CLI arguments parsing

    Args:
        argv (list): list of args to be parsed

    Returns:
        ArgumentParser: Parsed arguments
    """
    parser = argparse.ArgumentParser(
        description='Generate O_C and Hemisphere app list',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--src', help='Main scr path (where OC_apps.cpp and hemisphere_config.h are located)',
                        default='./software/src/')

    res = parser.parse_args(argv[1:])

    return res

if __name__ == "__main__":
    main(sys.argv)
