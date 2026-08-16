#include <stdint.h>
#include "../kernel/multiboot.h"
#include "../mm/vmm.h"
uint32_t   *framebuffer  = 0;
int         screen_w     = 0;
int         screen_h     = 0;
int         screen_pitch = 0;
int         screen_bpp   = 0;
extern void vmm_map_page(void *phys, void *virt, int flags);

/*
 * Sets the color of a single pixel at the given (x, y) coordinates.
 * We calculate the linear offset in the framebuffer by multiplying the y 
 * coordinate by the row length (pitch in dwords) and adding the x coordinate.
 * Bounds checking prevents writing outside the mapped framebuffer memory.
 */
void        put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= screen_w || y < 0 || y >= screen_h)
        return;
    uint32_t index     = y * (screen_pitch / 4) + x;
    framebuffer[index] = color;
}

/*
 * Fills the entire framebuffer with a solid color.
 * Iterates through every pixel on the screen and sets its color value.
 * This is typically used to clear the screen before redrawing.
 */
void graphics_clear(uint32_t color) {
    uint32_t size = screen_w * screen_h;
    for (uint32_t i = 0; i < size; i++)
        framebuffer[i] = color;
}

/*
 * Initializes the graphics subsystem using multiboot information.
 * Checks the multiboot flags for valid framebuffer data (bit 12). If found, 
 * extracts resolution and pitch, then maps the physical framebuffer address 
 * to virtual memory. The screen is cleared to black upon successful setup.
 */
void init_graphics(multiboot_info_t *mboot) {
    if (!(mboot->flags & (1 << 12)))
        return;
    screen_w         = mboot->framebuffer_width;
    screen_h         = mboot->framebuffer_height;
    screen_pitch     = mboot->framebuffer_pitch;
    screen_bpp       = mboot->framebuffer_bpp;
    uint32_t fb_phys = (uint32_t)mboot->framebuffer_addr;
    uint32_t fb_size = screen_h * screen_pitch;
    for (uint32_t offset = 0; offset < fb_size; offset += 4096) {
        vmm_map_page((void *)(fb_phys + offset), (void *)(fb_phys + offset), 0x3);
    }
    framebuffer = (uint32_t *)fb_phys;
    graphics_clear(0xFF000000);
}