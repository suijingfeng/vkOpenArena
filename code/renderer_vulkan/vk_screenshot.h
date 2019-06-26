#ifndef VK_SCREENSHOT_H_
#define VK_SCREENSHOT_H_

void R_ScreenShotJPEG_f(void);
void R_ScreenShot_f( void );

void RB_TakeScreenshot( uint32_t width, uint32_t height, char * const fileName, VkBool32 isJpeg);

// void vk_createScreenShotBuffer(uint32_t howMuch);
// void vk_destroyScreenShotBuffer(void);
#endif
