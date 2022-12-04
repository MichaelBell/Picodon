# Picodon <!-- omit in toc -->

Picodon: A Mastodon client for the Pico W.

This works with a [VGA demo board](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base?variant=32369520672851), the idea is to display recent toots from your home timeline on a VGA display at 720p.

This is not currently easily usable, because it relies on being able to fetch a <38kB 240x240 jpeg version of avatar from the Mastodon server by tweaking the normal avatar URL.  It would be fairly straightforward to make a relay that does this locally - I'll add instructions to do that in future.

## Acknowledgments

The [TLS client](https://github.com/peterharperuk/pico-examples/tree/add_mbedtls_example/pico_w/tls_client) is based on the example in Peter Harper's branch of pico-examples.

The [jpeg decoder](https://github.com/bitbank2/JPEGDEC) is from Larry Bank, used under the APLv2.

[cJSON](https://github.com/DaveGamble/cJSON) is a light weight C JSON parser, used under the MIT license.

Fonts from the Raspberry Pi [textmode example](https://github.com/raspberrypi/pico-playground/tree/master/scanvideo/textmode), which are originally from Ubuntu, used under the Ubuntu Font Licence.

I've added the files directly rather than using submodules as it is simpler to make small tweaks this way, and they are only a couple of files from larger repos.
