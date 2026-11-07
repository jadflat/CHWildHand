"""Wild Hand: what the shared CHGame tools need to know (`cl run` and `cl gif` read this)."""

# One framebuffer row a DMA chunk (CHGfx's default is 2): about 500 B of RAM back; measured at 20 ms a frame on the board.
DEFINES = ["GFX_CHUNK_ROWS=1"]
TESTS = {}
SAVE_PAGES = 1   # one save page: the image is over 50,432 B (the A/B pair needs it); a power cut mid-save can lose one save

# Every script needs WILDHAND.DAT (the pictures and songs are on the SD card): tools/make_card.py writes it.
CARD = dict(scripts="", file="sdcard/WILDHAND.DAT", build=["python", "tools/make_card.py"])
