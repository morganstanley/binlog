import re
import os

from markdown import Extension
from markdown.preprocessors import Preprocessor

class CatchfileExtension(Extension):
    """
    Extension for Python-Markdown - replace markers with file content

    Usage:

    Include whole example.cpp file:

        [catchfile example.cpp]

    Include only parts of example.cpp surrounded by //[my_snippet and //]:

        [catchfile example.cpp my_snippet]

    The specified file name must be relative to `base_dir`.
    Prepends :::lang marker to code blocks for known extensions.
    If a marker appears multiple times in a single file, every marked
    range gets included, in order.
    Whitespace indent of the included content is replaced by
    the indent of the marker in the markdown document.
    """

    def __init__(self, **kwargs):
        self.config = {
          'base_dir' : ['', 'Directory of files with content to be included']
        }
        Extension.__init__(self, **kwargs)

    def extendMarkdown(self, md, md_globals):
        # Insert CatchfilePreprocessor before ReferencePreprocessor.
        md.preprocessors.add('catchfile', CatchfilePreprocessor(self.getConfigs()), '<html_block')

class CatchfilePreprocessor(Preprocessor):
    def __init__(self, config):
        self.base_dir = config['base_dir']
        self.tag_re = re.compile(r'^(\s*)\[catchfile ([^ ]+)(?: ([^ ]+))?\]$')

    def run(self, lines):
        new_text = []
        for line in lines:
            m = self.tag_re.match(line)
            if m:
                # get arguments
                indent = m.groups()[0]
                path = m.groups()[1]
                marker = m.groups()[2]

                if marker:
                  content = self._get_snippet(path, marker)
                else:
                  content = self._get_file(path)

                # adjust the indent of the content to the indent of the marker
                line = indent + content.replace('\n', '\n' + indent)

            new_text.append(line)

        return new_text

    def _get_snippet(self, path, marker):
        result = ''
        mark_begin = '//[' + marker
        mark_end = '//]'
        fullpath = os.path.join(self.base_dir, path)

        with open(fullpath, 'r') as lines:
            apping = False
            prefix = ''
            for line in lines:
                if not apping and line.strip() == mark_begin:
                    apping = True
                    prefix = self._get_marker_prefix(line)
                elif apping and line.strip() == mark_end:
                    apping = False
                elif apping:
                    result += self._unprefix(line, prefix)
        if len(result) == 0:
            raise KeyError("Marker not found: '%s' in file: '%s'" % (marker,fullpath))
        return result

    def _get_marker_prefix(self, marker_line):
        marker = marker_line.lstrip()
        pref_len = len(marker_line) - len(marker)
        return marker_line[:pref_len]

    def _unprefix(self, line, prefix):
        if line.startswith(prefix):
            return line[len(prefix):]
        return line

    def _get_file(self, path):
        with open(os.path.join(self.base_dir, path), 'r') as f:
            result = f.read()
        return result
