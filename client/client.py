import pygame # 后续要拆分pygame导入
import socket # 后续拆分socket导入
import threading
import sys
from abc import ABC, abstractmethod
# -*- encoding: utf-8 -*-

class InteractorAreaFactor(ABC):
    @abstractmethod
    def construct():...
    
    @abstractmethod
    def blit():...
    
    
pygame.font.init()
button_rect = pygame.Rect(300, 250, 200, 50)
button_color = (255, 255, 255)
border_color = (0, 0, 0)
text_color = (0, 0, 0)
font = pygame.font.Font(None, 36)
button_text = font.render("Click Me", True, text_color)

# Draw button function
def draw_button(surface: pygame.Surface):
    pygame.draw.rect(surface, button_color, button_rect)
    pygame.draw.rect(surface, border_color, button_rect, 2)
    surface.blit(button_text,
    (button_rect.centerx - button_text.get_width() // 2,
    button_rect.centery - button_text.get_height() // 2))

# Event handler function
def handle_events():
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return False
            if event.type == pygame.MOUSEBUTTONDOWN:
                if button_rect.collidepoint(event.pos):
                    print("Button clicked!")
                    return True
    
    

def welcome_screen(surface: pygame.Surface):
    # 欢迎界面
    global RUNNING
    surface.fill((255, 255, 255))
    
    draw_button(surface)
    pygame.display.flip()
    RUNNING = handle_events()
    
    
    

def game_screen(surface: pygame.Surface):
    # 游戏界面
    pass

WELCOMEFLAG = True
GAMEFLAG = False
RUNNING = True
def main():
    # 主程序
    global RUNNING
    pygame.init()
    screen = pygame.display.set_mode((1280, 720))
    pygame.display.set_caption("斗地主")
    clock = pygame.time.Clock()
    
    try:
        while RUNNING:
            events = pygame.event.get()
            for event in events:
                if event.type == pygame.QUIT:
                    RUNNING = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        RUNNING = False
                # 可选：处理窗口大小变化等其他事件
            
            # 如果窗口被强制关闭（某些系统可能不发送QUIT事件）
            if not pygame.display.get_init():
                RUNNING = False
            
            # 游戏更新和绘制
            screen.fill((255, 255, 255))            
            pygame.display.flip()
            welcome_screen(screen)

            clock.tick(60)
            
    except Exception as e:
        print(f"程序异常: {e}")
    finally:
        # 确保资源被正确释放
        pygame.quit()
        sys.exit()

if __name__ == "__main__":
    main()