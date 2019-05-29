import sys
import os
import argparse
import time
import markdown
import pygments

from catchfile import CatchfileExtension

def main():
  parser = argparse.ArgumentParser(description='Read a markdown file from stdin, print a HTML file to stdout')

  parser.add_argument('-c', '--catchfile-dir', default='.', help='Directory of files to be included by catchfile')
  args = parser.parse_args()

  catchfile = CatchfileExtension(base_dir = args.catchfile_dir)

  extensions = ['toc', 'codehilite', 'meta', catchfile]
  extension_configs = {
      'toc' : [('anchorlink', True)],
  }

  md = markdown.Markdown(extensions=extensions, extension_configs=extension_configs)

  md_input = sys.stdin.read()
  html_output = md.convert(md_input)

  with open ('body.html', 'r') as template:
      processed = template.read() \
        .replace('{% date-generated %}', time.strftime('%Y. %m. %d.')) \
        .replace('{% title %}', md.Meta['title'][0]) \
        .replace('{% content %}', html_output)

      print(processed)

if __name__ == '__main__':
    main()
