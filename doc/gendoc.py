import sys
import os
import argparse
import time
import markdown

from catchfile import CatchfileExtension
from codelink import CodelinkExtension

def main():
  parser = argparse.ArgumentParser(description='Read a markdown file from stdin, print a HTML file to stdout')

  parser.add_argument('-c', '--catchfile-dir', default='.', help='Directory of files to be included by catchfile')
  parser.add_argument('-s', '--source-browser-base-url', help='URL prefix of links in code snippets')
  args = parser.parse_args()

  catchfile = CatchfileExtension(base_dir = args.catchfile_dir)

  extensions = ['toc', 'meta', catchfile]

  # only add links to code if there's a source browser configured
  if args.source_browser_base_url:
    codelink = CodelinkExtension(link_prefix = args.source_browser_base_url)
    extensions.append(codelink)

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
