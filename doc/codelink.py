import re

from markdown import Extension
from markdown.treeprocessors import Treeprocessor
from markdown.util import AtomicString

class CodelinkExtension(Extension):
    """
    Extension for Python-Markdown - add links to #include statements

    Given the following code snippet:

        #include <foo/bar.hpp>
        foo::bar();

    It makes the "foo/bar.hpp" part of the first line clickable,
    linking to `link_prefix`foo/bar.hpp, where `link_prefix`
    is user specified.
    """

    def __init__(self, **kwargs):
        self.config = {
          'link_prefix' : ['', 'Location of the code repository']
        }
        Extension.__init__(self, **kwargs)

    def extendMarkdown(self, md, md_globals):
        md.treeprocessors.add('codelink', CodelinkProcessor(md, self.getConfigs()), '>prettify')

class CodelinkProcessor(Treeprocessor):
    def __init__(self, md, config):
        self.md = md
        self.config = config
        self.include = re.compile('#include &lt;(.+?\.hpp)&gt;')

    def run(self, root):
        prefix = self.config['link_prefix']
        for elem in root.getiterator('code'):
            code = elem.text.replace('<', '&lt;').replace('>', '&gt;')
            code = self.include.sub(r'#include &lt;<a href="' + prefix + r'\1">\1</a>&gt;', code)
            placeholder = self.md.htmlStash.store(code)
            elem.text = placeholder
