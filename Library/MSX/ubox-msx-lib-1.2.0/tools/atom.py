#!/usr/bin/env python3

# script to generate an atom feed from CHANGES.md

import re
from datetime import datetime, timezone, time

release = re.compile("^## Release ([0-9]+\\.[0-9]+\\.[0-9]+) - ([0-9]+-[0-9]+-[0-9]+)$")

title = "ubox MXS lib releases"
base_url = "https://www.usebox.net/jjm/ubox-msx-lib/changes.html"
release_url = "https://git.usebox.net/ubox-msx-lib/tag/"
feed_file = "releases.xml"
author = "jjm"
tz = timezone.utc


def get_entries(changes):
    entries = []

    with open(changes, "rt") as fd:
        lines = fd.readlines()

    i = 0
    while i < len(lines):
        line = lines[i]
        m = release.match(line)
        if m:
            version = m.group(1)
            # because timezones and summer time, use 1am
            updated_on = datetime.combine(
                datetime.fromisoformat(m.group(2)), time(1, 0, 0)
            ).astimezone(tz)
            body = ""
            i += 1
            while i < len(lines) and (lines[i] == "" or lines[i][0] != "#"):
                body += lines[i]
                i += 1

            entries.append(dict(version=version, updated_on=updated_on, body=body))
        else:
            i += 1

    entries.sort(key=lambda o: o["updated_on"], reverse=True)
    return entries


def to_atom(post):
    return """\
<entry>
    <title>{title}</title>
    <link rel="alternate" href="{url}"/>
    <id>{url}</id>
    <updated>{updated_on}</updated>
    <content type="xhtml">
      <div xmlns="http://www.w3.org/1999/xhtml">
      <pre>{body}</pre>
      <p>Download available at <a href="{release_url}?h={version}">ubox MSX lib website</a>.</p>
      </div>
    </content>
</entry>\n""".format(
        title="Released ubox MSX lib " + post["version"],
        url=base_url + "#release-" + post["version"] + "---" + post["updated_on"].date().isoformat(),
        version=post["version"],
        release_url=release_url,
        updated_on=post["updated_on"].isoformat(),
        body=post["body"].rstrip() + "\n",
    )


def gen_atom(entries):

    updated_on = datetime(1900, 1, 1, tzinfo=tz)
    posts_atom = []
    for post in entries:
        if post["updated_on"] > updated_on:
            updated_on = post["updated_on"]

        posts_atom.append(to_atom(post))

    return "\n".join(
        [
            """<?xml version="1.0" encoding="utf-8"?>""",
            """<feed xmlns="http://www.w3.org/2005/Atom">""",
            """<title>%s</title>""" % title,
            """<link href="%s"/>""" % base_url,
            """<link rel="self" href="%s"/>""" % (base_url + feed_file),
            """<updated>%s</updated>""" % updated_on.isoformat(),
            """<id>%s</id>""" % base_url,
            """<author>""",
            """    <name>%s</name>""" % author,
            """</author>""",
        ]
        + posts_atom
        + ["</feed>"]
    )


def main():
    entries = get_entries("../CHANGES.md")
    with open(feed_file, "wt") as fd:
        fd.write(gen_atom(entries))


if __name__ == "__main__":
    main()
