# Islandora OAI Download Link

Islandora utility module that adds a custom request handler for the Islandora OAI module. Currently under heavy development, do not use yet.

## Requirements

* [Islandora OAI](https://github.com/Islandora/islandora_oai)

## Usage

This module adds to the default response handler provided by Islandora OAI. To use it,

1. Select the "Islandora OAI with download link" request handler in the Islandora OAI admin settings at `admin/islandora/tools/islandora-oai`. 
1. Save the configuration.
1. Click on the request handler's 'configure' link.
1. In the new "Download links" fieldset, configure your "Content model to link mappings for download links" and ranges of allowed IP addresses.

The changes this module makes to the default OAI metadata are limited to adding the elements containig the direct download links, and requests that do not fall within allowed IP ranges or for objects not configured in a mapping remain unaffected.


# Maintainer

* [Mark Jordan](https://github.com/mjordan)

## Development and feedback

Pull requests are welcome, as are use cases and suggestions. Please open an issue before creating a pull request.

## License

* [GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)
