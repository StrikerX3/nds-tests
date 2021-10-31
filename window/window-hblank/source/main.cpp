#include <nds.h>

#include <stdio.h>

// clang-format off
u8 tiles[] =
{
// Tile 0: transparent tile
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
// Tile 1: solid tile using color index 1
	0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,
	0x11,0x11,0x11,0x11,
// Tile 2: solid tile using color index 2	
	0x22,0x22,0x22,0x22,
	0x22,0x22,0x22,0x22,
	0x22,0x22,0x22,0x22,
	0x22,0x22,0x22,0x22,
	0x22,0x22,0x22,0x22,
	0x22,0x22,0x22,0x22,
	0x22,0x22,0x22,0x22,
	0x22,0x22,0x22,0x22,
};

u16 palette[] = 
{
	RGB15(0,0,0),
	RGB15(31,0,0),
	RGB15(0,31,0)
};
// clang-format on

struct Window {
    u8 left = 0, top = 0;
    u8 right = 255, bottom = 192;
    bool enabled = false;
    bool bgEnable[2] = {false, false};
};

Window windows[2];

void onHBlank() {
    u16 vcount = (REG_VCOUNT + 1) % 262;
    for (auto &window : windows) {
        if (window.enabled && vcount >= window.top && vcount <= window.bottom) {
            windowSetBounds(WINDOW_0, window.left, vcount, window.right, vcount + 1);
            break;
        }
    }
}

int main(int argc, char **argv) {
    consoleDemoInit();

    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);

    int bg0 = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
    int bg1 = bgInit(1, BgType_Text4bpp, BgSize_T_256x256, 1, 1);

    dmaCopy(tiles, bgGetGfxPtr(bg0), sizeof(tiles));
    dmaFillWords(0x00010001, bgGetMapPtr(bg0), 32 * 32 * 2);
    dmaFillWords(0x00020002, bgGetMapPtr(bg1), 32 * 32 * 2);
    dmaCopy(palette, BG_PALETTE, sizeof(palette));

    windows[0].enabled = true;
    windows[0].left = 64;
    windows[0].top = 64;
    windows[0].right = 192;
    windows[0].bottom = 128;
    windows[0].bgEnable[0] = true;
    windows[0].bgEnable[1] = false;

    windows[1].enabled = true;
    windows[1].left = 32;
    windows[1].top = 32;
    windows[1].right = 224;
    windows[1].bottom = 160;
    windows[1].bgEnable[0] = false;
    windows[1].bgEnable[1] = true;

    size_t windowIndex = 0;

    bgWindowEnable(0, WINDOW_0);

    unsigned int priority = 0;

    keysSetRepeat(20, 2);

    irqEnable(IRQ_HBLANK);
    irqSet(IRQ_HBLANK, onHBlank);

    while (1) {
        swiWaitForVBlank();

        scanKeys();
        int keys = keysDown();
        int held = keysHeld();
        int repeat = keysDownRepeat();
        if (keys & KEY_START) break;

        if (keys & KEY_L) windowIndex = (windowIndex + 1) % (sizeof(windows) / sizeof(windows[0]));
        if (keys & KEY_B) {
            priority ^= 1;
            bgSetPriority(0, priority);
        }
        if (keys & KEY_X) {
            windows[windowIndex].bgEnable[0] ^= true;
            if (windows[windowIndex].bgEnable[0]) {
                bgWindowEnable(0, WINDOW_0);
            } else {
                bgWindowDisable(0, WINDOW_0);
            }
        }
        if (keys & KEY_Y) {
            windows[windowIndex].bgEnable[1] ^= true;
            if (windows[windowIndex].bgEnable[1]) {
                bgWindowEnable(1, WINDOW_0);
            } else {
                bgWindowDisable(1, WINDOW_0);
            }
        }
        if (repeat & KEY_LEFT) {
            if (held & KEY_R) {
                windows[windowIndex].left--;
            } else {
                windows[windowIndex].right--;
            }
        }
        if (repeat & KEY_RIGHT) {
            if (held & KEY_R) {
                windows[windowIndex].left++;
            } else {
                windows[windowIndex].right++;
            }
        }
        if (repeat & KEY_UP) {
            if (held & KEY_R) {
                windows[windowIndex].top--;
            } else {
                windows[windowIndex].bottom--;
            }
        }
        if (repeat & KEY_DOWN) {
            if (held & KEY_R) {
                windows[windowIndex].top++;
            } else {
                windows[windowIndex].bottom++;
            }
        }
        if (keys & KEY_SELECT) windows[windowIndex].enabled ^= true;

        bool hasWindow = false;
        for (auto &window : windows) {
            if (window.enabled) {
                hasWindow = true;
                break;
            }
        }
        if (hasWindow) {
            windowEnable(WINDOW_0);
        } else {
            windowDisable(WINDOW_0);
        }

        consoleClear();
        for (size_t i = 0; i < 2; i++) {
            printf("%c W%d: %ux%u - %ux%u (%s)\n", (windowIndex == i) ? '>' : ' ', i, windows[i].left, windows[i].top,
                   windows[i].right, windows[i].bottom, windows[i].enabled ? "on" : "off");
        }
        printf("\n");
        printf("[D-pad]  Move bottom-right\n");
        printf("  [+R]   Move top-left\n");
        printf("[SELECT] Toggle window on/off\n");
        printf("[L]      Switch window\n");
        printf("[X]      BG0 window: %s %s\n", windows[0].bgEnable[0] ? "on" : "off",
               windows[1].bgEnable[0] ? "on" : "off");
        printf("[Y]      BG1 window: %s %s\n", windows[0].bgEnable[1] ? "on" : "off",
               windows[1].bgEnable[1] ? "on" : "off");
        printf("[B]      BG0 priority: %s\n", priority ? "back" : "front");
        // printf("\n");
        // printf("%p %p\n", bgGetGfxPtr(bg0), bgGetGfxPtr(bg1));
        // printf("%p %p\n", bgGetMapPtr(bg0), bgGetMapPtr(bg1));
    }

    return 0;
}
