---
title: atma documentation
---

<figure markdown="span">

![atma](atma.png){ width="70%" }

</figure>

A cross-platform retro engine targeting classic computers, which also builds
on modern PCs.

This site mixes handwritten guides with API reference generated from the C
sources. The API pages under **API** are produced by Doxygen and doxybook2 and
are rebuilt by `make docs`; do not edit them by hand.

## Building the docs

```
make docs        # regenerate docs/api, then build the site into build/docs/site
make docs-serve  # regenerate docs/api, then serve with live reload
make docs-clean  # remove generated XML, Markdown and site output
```
