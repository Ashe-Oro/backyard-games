# Backyard Games Web — Changelog

All notable changes to the website (home page + five yard games, mobile and desktop browsers)
are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project follows [Semantic Versioning](https://semver.org/).

Before `web-v1.0.0`, the site deployed continuously from `main`; see the merged pull requests for that history.

## [Unreleased]

### Fixed
- Line up left or right of center and throw straight: the bag flies straight and lands to that side of the hole.
- Straight throws no longer auto-aim at the hole.

### Changed
- The site's files moved into `web/`, which Vercel now serves (`"outputDirectory": "web"`). Pages and URLs are unchanged.
