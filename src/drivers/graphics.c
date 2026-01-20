/* src/drivers/graphics.c */
#include <stdint.h>
#include "../kernel/multiboot.h"
#include "../mm/vmm.h" // Ensures vmm_map_page is available

uint32_t* framebuffer = 0;
int screen_w = 0;
int screen_h = 0;
int screen_pitch = 0; 
int screen_bpp = 0;

// VMM Helper defined in vmm.c
extern void vmm_map_page(void* phys, void* virt, int flags);

void put_pixel(int x, int y, uint32_t color) {
    // Basic bounds check
    if (x < 0 || x >= screen_w || y < 0 || y >= screen_h) return;
    
    // Calculate offset: y * (bytes_per_row) + x * (bytes_per_pixel)
    // We assume 32-bpp (4 bytes) for simplicity here. 
    // If pitch is in bytes, we divide by 4 to get uint32 index.
    uint32_t index = y * (screen_pitch / 4) + x;
    framebuffer[index] = color;
}

// Function to clear screen fast
void graphics_clear(uint32_t color) {
    uint32_t size = screen_w * screen_h;
    for(uint32_t i=0; i<size; i++) framebuffer[i] = color;
}

void init_graphics(multiboot_info_t* mboot) {
    if (!(mboot->flags & (1 << 12))) return; // No VBE info

    screen_w = mboot->framebuffer_width;
    screen_h = mboot->framebuffer_height;
    screen_pitch = mboot->framebuffer_pitch;
    screen_bpp = mboot->framebuffer_bpp;
    
    uint32_t fb_phys = (uint32_t)mboot->framebuffer_addr;
    uint32_t fb_size = screen_h * screen_pitch;
    
    // Map the Framebuffer into Virtual Memory
    // We Map it 1:1 (Virt = Phys) for simplicity, or to a high address.
    // Mapping 1:1 is dangerous if it overlaps kernel code, but usually FB is at 0xE0000000+
    
    for (uint32_t offset = 0; offset < fb_size; offset += 4096) {
        // Map as Kernel RW (0x3 = Present | RW)
        // If we want User to draw directly (bad idea), use 0x7.
        vmm_map_page((void*)(fb_phys + offset), (void*)(fb_phys + offset), 0x3); 
    }

    framebuffer = (uint32_t*)fb_phys;
    
    // Clear to black
    graphics_clear(0xFF000000);
}