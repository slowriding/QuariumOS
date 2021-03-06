#include "kernel.h"
#include "utils.h"
#include "keyboard.h"
#include "str.h"
#include "box.h"
#include <stdint.h>
#include <stddef.h>
#include "calc.h"
#include "structapp.h"
#define VGA_ADDRESS 0xB8000
#define BUFSIZE 2200

uint16 *vga_buffer;
extern uint32 vga_index;
//extern vga_index;
extern int goprotectedmod();

void memory_copy(uint8 *source, uint8 *dest, int nbytes)
{
  int i;
  for (i = 0; i < nbytes; i++)
  {
    *(dest + i) = *(source + i);
  }
}

void memory_set(uint8 *dest, uint8 val, uint32 len)
{
  uint8 *temp = (uint8 *)dest;
  for (; len != 0; len--)
    *temp++ = val;
}

/* This should be computed at link time, but a hardcoded
 * value is fine for now. Remember that our kernel starts
 * at 0x1000 as defined on the Makefile */
uint32_t free_mem_addr = 0x10000;
/* Implementation is just a pointer to some free memory which
 * keeps growing */
uint32_t kmalloc(size_t size, int align, uint32_t *phys_addr)
{
  /* Pages are aligned to 4K, or 0x1000 */
  if (align == 1 && (free_mem_addr & 0xFFFFF000))
  {
    free_mem_addr &= 0xFFFFF000;
    free_mem_addr += 0x1000;
  }
  /* Save also the physical address */
  if (phys_addr)
    *phys_addr = free_mem_addr;

  uint32_t ret = free_mem_addr;
  free_mem_addr += size; /* Remember to increment the pointer */
  return ret;
}
void hex_to_ascii(int n, char str[])
{
  append(str, '0');
  append(str, 'x');
  char zeros = 0;

  __INT32_TYPE__ tmp;
  int i;
  for (i = 28; i > 0; i -= 4)
  {
    tmp = (n >> i) & 0xF;
    if (tmp == 0 && zeros == 0)
      continue;
    zeros = 1;
    if (tmp > 0xA)
      append(str, tmp - 0xA + 'a');
    else
      append(str, tmp + '0');
  }

  tmp = n & 0xF;
  if (tmp >= 0xA)
    append(str, tmp - 0xA + 'a');
  else
    append(str, tmp + '0');
}

/* K&R */
void reverse(char s[])
{
  int c, i, j;
  for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
  {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

void backspace(char s[])
{
  int len = strlen(s);
  s[len - 1] = '\0';
}
uint16 get_box_draw_char(uint8 chn, uint8 fore_color, uint8 back_color)
{
  uint16 ax = 0;
  uint8 ah = 0;

  ah = back_color;
  ah <<= 4;
  ah |= fore_color;
  ax = ah;
  ax <<= 8;
  ax |= chn;

  return ax;
}

void draw_generic_box(uint16 x, uint16 y,
                      uint16 width, uint16 height,
                      uint8 fore_color, uint8 back_color,
                      uint8 topleft_ch,
                      uint8 topbottom_ch,
                      uint8 topright_ch,
                      uint8 leftrightside_ch,
                      uint8 bottomleft_ch,
                      uint8 bottomright_ch)
{
  uint32 i;

  //increase vga_index to x & y location
  vga_index = 80 * y;
  vga_index += x;

  //draw top-left box character
  vga_buffer[vga_index] = get_box_draw_char(topleft_ch, fore_color, back_color);

  vga_index++;
  //draw box top characters, -
  for (i = 0; i < width; i++)
  {
    vga_buffer[vga_index] = get_box_draw_char(topbottom_ch, fore_color, back_color);
    vga_index++;
  }

  //draw top-right box character
  vga_buffer[vga_index] = get_box_draw_char(topright_ch, fore_color, back_color);

  // increase y, for drawing next line
  y++;
  // goto next line
  vga_index = 80 * y;
  vga_index += x;

  //draw left and right sides of box
  for (i = 0; i < height; i++)
  {
    //draw left side character
    vga_buffer[vga_index] = get_box_draw_char(leftrightside_ch, fore_color, back_color);
    vga_index++;
    //increase vga_index to the width of box
    vga_index += width;
    //draw right side character
    vga_buffer[vga_index] = get_box_draw_char(leftrightside_ch, fore_color, back_color);
    //goto next line
    y++;
    vga_index = 80 * y;
    vga_index += x;
  }
  //draw bottom-left box character
  vga_buffer[vga_index] = get_box_draw_char(bottomleft_ch, fore_color, back_color);
  vga_index++;
  //draw box bottom characters, -
  for (i = 0; i < width; i++)
  {
    vga_buffer[vga_index] = get_box_draw_char(topbottom_ch, fore_color, back_color);
    vga_index++;
  }
  //draw bottom-right box character
  vga_buffer[vga_index] = get_box_draw_char(bottomright_ch, fore_color, back_color);

  vga_index = 0;
}

void draw_box(uint8 boxtype,
              uint16 x, uint16 y,
              uint16 width, uint16 height,
              uint8 fore_color, uint8 back_color)
{
  switch (boxtype)
  {
  case BOX_SINGLELINE:
    draw_generic_box(x, y, width, height,
                     fore_color, back_color,
                     218, 196, 191, 179, 192, 217);
    break;

  case BOX_DOUBLELINE:
    draw_generic_box(x, y, width, height,
                     fore_color, back_color,
                     201, 205, 187, 186, 200, 188);
    break;
  }
}

void fill_box(uint8 ch, uint16 x, uint16 y, uint16 width, uint16 height, uint8 color)
{
  uint32 i, j;

  for (i = 0; i < height; i++)
  {
    //increase vga_index to x & y location
    vga_index = 80 * y;
    vga_index += x;

    for (j = 0; j < width; j++)
    {
      vga_buffer[vga_index] = get_box_draw_char(ch, 0, color);
      vga_index++;
    }
    y++;
  }
}

char VERSION[] = "0.0.1";
uint32 vga_index;
static uint32 next_line_index = 1;
uint8 g_fore_color = WHITE, g_back_color = BLUE;
int digit_ascii_codes[10] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};

uint16 vga_entry(unsigned char ch, uint8 fore_color, uint8 back_color)
{
  uint16 ax = 0;
  uint8 ah = 0, al = 0;

  ah = back_color;
  ah <<= 4;
  ah |= fore_color;
  ax = ah;
  ax <<= 8;
  al = ch;
  ax |= al;

  return ax;
}

void clear_vga_buffer(uint16 **buffer, uint8 fore_color, uint8 back_color)
{
  uint32 i;
  for (i = 0; i < BUFSIZE; i++)
  {
    (*buffer)[i] = vga_entry(NULL, fore_color, back_color);
  }
  next_line_index = 1;
  vga_index = 0;
}

void init_vga(uint8 fore_color, uint8 back_color)
{
  vga_buffer = (uint16 *)VGA_ADDRESS;
  clear_vga_buffer(&vga_buffer, fore_color, back_color);
  g_fore_color = fore_color;
  g_back_color = back_color;
}

void clear_screen(uint8 fore_color, uint8 back_color)
{
  clear_vga_buffer(&vga_buffer, fore_color, back_color);
}
void clear_FS() { print_string("test"); }
void clear_FC(struct app app)
{
  clear_screen(BLACK, RED);
  gotoxy(5, 0);
  char strs[] = "Q-DOS: ";
  char bar[] = "";
  //print_char((int) 196);

  draw_box(BOX_SINGLELINE, 0, 0, 78, 23, BLACK, RED);
  print_color_string(strs, BLACK, RED);
  print_color_string(VERSION, BLACK, RED);
  //print_color_string(bar, BLACK, RED);
  gotoxy(30, 0);
  print_color_string(app.name, BLACK, app.backcolor);
  //print_color_string(app.version, BLACK, app.backcolor);

  print_new_line();
  return;
}
void clear_F()
{
  clear_screen(BLACK, RED);
  gotoxy(5, 0);
  char strs[] = "Q-DOS:";

  draw_box(BOX_SINGLELINE, 0, 0, 78, 23, BLACK, RED);
  print_color_string(strs, BLACK, RED);
  print_color_string(VERSION, BLACK, RED);
  print_new_line();
  return;
}
void print_new_line()
{
  if (next_line_index >= 55)
  {
    next_line_index = 0;
    clear_vga_buffer(&vga_buffer, g_fore_color, g_back_color);
  }
  vga_index = 80 * next_line_index;
  vga_index += 1;
  next_line_index++;
}

void print_char(char ch)
{
  if (BUFSIZE - 200 <= vga_index)
  {
    clear_screen(BLACK, RED);
    gotoxy(0, 0);
  }
  vga_buffer[vga_index] = vga_entry(ch, g_fore_color, g_back_color);
  vga_index++;
}

void print_string(char *str)
{
  uint32 index = 0;
  while (str[index])
  {
    print_char(str[index]);
    index++;
  }
}

void print_color_string(char *str, uint8 fore_color, uint8 back_color)
{
  uint32 index = 0;
  uint8 fc, bc;
  fc = g_fore_color;
  bc = g_back_color;
  g_fore_color = fore_color;
  g_back_color = back_color;
  while (str[index])
  {
    print_char(str[index]);
    index++;
  }
  g_fore_color = fc;
  g_back_color = bc;
}

void print_int(int num)
{
  char str_num[digit_count(num) + 1];
  itoa(num, str_num);
  print_string(str_num);
}

uint8 inb(uint16 port)
{
  uint8 ret;
  asm volatile("inb %1, %0"
               : "=a"(ret)
               : "d"(port));
  return ret;
}

void outb(uint16 port, uint8 data)
{
  asm volatile("outb %0, %1"
               : "=a"(data)
               : "d"(port));
}
byte get_input_keycodemath()
{
  uint16 oldvgai = vga_index;
  int curs = 0;
  int a = 0;
  int j = 0;
  byte keycode = 0;
  for(keycode = inb(KEYBOARD_PORT);TRUE;curs=0)
  {
    vga_index = oldvgai;
    if (keycode > 0)
    {
      a = 1;
    }

    if (j == 200000 / 10)
    {
      print_color_string("|", BLACK, RED);
      j = 0;
    }
    if (j == 100000 / 10)
    {
      print_color_string("_", RED, RED);
    }
    j++;
    //if ((int)a==KEY_EQUAL){
      //a=107;
    //}
      if (isin(get_char(a),"1234567890x/-+")){
        print_color_string("_", RED, RED);
      vga_index = oldvgai;
      return keycode;}
    
  }
  
}
byte get_input_keycode()
{
  uint16 oldvgai = vga_index;
  int curs = 0;
  int a = 0;
  int j = 0;
  byte keycode = 0;
  while ((keycode = inb(KEYBOARD_PORT)) != 0)
  {
    vga_index = oldvgai;
    if (keycode > 0)
    {
      a = 1;
    }

    if (j == 200000 / 10)
    {
      print_color_string("|", BLACK, RED);
      j = 0;
    }
    if (j == 100000 / 10)
    {
      print_color_string("_", RED, RED);
    }
    j++;

    if (a)
    {
      print_color_string("_", RED, RED);
      vga_index = oldvgai;
      return keycode;
    }
  }
  return keycode;
}

/*
keep the cpu busy for doing nothing(nop)
so that io port will not be processed by cpu
here timer can also be used, but lets do this in looping counter
*/
void wait_for_io(uint32 timer_count)
{
  while (1)
  {
    asm volatile("nop");
    timer_count--;
    if (timer_count <= 0)
      break;
  }
}

void sleep(uint32 timer_count)
{
  wait_for_io(timer_count);
}

void gotoxy(uint16 x, uint16 y)
{
  vga_index = 80 * y;
  vga_index += x;
  vga_index += 1;
}
char get_char(byte c)
{
  if (c == KEY_A)
  {
    return 'a';
  }
  else if (c == KEY_B)
  {
    return 'b';
  }
  else if (c == KEY_C)
  {
    return 'c';
  }
  else if (c == KEY_D)
  {
    return 'd';
  }
  else if (c == KEY_E)
  {
    return 'e';
  }
  else if (c == KEY_F)
  {
    return 'f';
  }
  else if (c == KEY_G)
  {
    return 'g';
  }
  else if (c == KEY_ENTER)
  {
    return '\n';
  }
  else if (c == KEY_H)
  {
    return 'h';
  }
  else if (c == KEY_I)
  {
    return 'i';
  }
  else if (c == KEY_J)
  {
    return 'j';
  }
  else if (c == KEY_F12)
  {
    return (char)28;
  }
  else if (c == KEY_BACKSPACE)
  {
    return (char)8;
  }
  else if (c == KEY_S)
  {
    return 's';
  }
  else if (c == KEY_L)
  {
    return 'l';
  }
  else if (c == KEY_X)
  {
    return 'x';
  }
  else if (c == KEY_T)
  {
    return 't';
  }
  else if (c == KEY_1)
  {
    return '1';
  }
  else if (c == KEY_R)
  {
    return 'r';
  }
  else if (c == KEY_U)
  {
    return 'u';
  }
  //int v=c+100;
  //print_int(c);
  //return (char) v;/*81*/

  else
  {
    return '\0';
  }
}
/*void go_to_protected(){
  asm
}*/

void kernel_entry()
{
  add_allcommandes();
  //extern go_to_protected();
  init_vga(BLACK, RED);
  clear_screen(BLACK, RED);
  byte ans = KEY_Y;

  //print_char((char)48);
  print_string(" <--- this is the vga cursor but the real cursor is not that");
  sleep(999999999);
  //sleep(999999999);

  goprotectedmod();
  //sleep(999999999);

  //asm("");
  //asm("int 0x1A");
  int size = 0;
  char line[] = "";
  char abc[] = "abc";
  int l = 0;
  int z = 32; //X
  int v = 7;  //Y
  vga_index = 2200;
  if (BUFSIZE - 200 <= vga_index)
  {
    clear_screen(RED, BLACK);
    gotoxy(0, 0);
  }
  draw_box(BOX_DOUBLELINE, z, v, 16, 7, RED, BLACK);
  gotoxy(z + 1, v + 1);
  //vg=(char) 200;
  print_color_string("    Mutiny    ", RED, BLACK);
  gotoxy(z + 1, v + 2);
  print_string("      OS       ");
  gotoxy(z + 1, v + 3);
  print_string("               ");
  gotoxy(z + 1, v + 4);
  print_string("Kernel Versions:");
  gotoxy(z + 1, v + 5);
  print_string("     -0.0.1    ");
  gotoxy(z + 1, v + 6);
  print_string("               ");
  gotoxy(z + 1, v + 7);
  print_string("BY Slow_Riding ");
  //go_to_pro();
  //sleep(9999999999);
  clear_F();
  //clear_screen(WHITE, BLACK);
  //go_to_protected();
  //sleep(99999999999999999);
  bool Running = TRUE;
  //print_char((char)28);
  //print_int(exp(4,4));
  //print_new_line();
  //print_int(modulus(4,0.5));
  //print_new_line();
  while (Running)
  { /*for(int i=0;i<255;i++){
    print_int(i);
    print_char((char) i);
    print_char((char) 32);  }
    */
    //print_string(line);
    sleep(29304890 * 5);
    sleep(39304890);
    sleep(39304890);
    ans = get_input_keycode();
    sleep(39304890);
    if ((int)(get_char(ans)) == 28)
    {
      break;
    }
    else if (ans == KEY_ENTER)
    {
      //char fnlstr[256][256];
      //split(line,'a',&fnlstr);

      //for (int i=0;i<)
      //print_string("blblblb1");
      sleep(39304890);

      //print_new_line();
      /*if (strcomp("calculus",line)){
        calculus();
      }*/
      extern void execute_func(char *name);

      execute_func(line);

      if (strbegw("date", line))
      {
        //outb(0x0071,0x0a);
        byte thing = inb(0x71);
        char time[] = "";
        hex_to_ascii((int)thing, time);
        print_string(time);
      }
      else if (strbegw("clears", line))
      {
        clear_F();
      }
      else if (strbegw("ls", line))
      {
        print_string("ls command dont work\n");
      }
      else if (strcomp("exit", line))
      {
        Running = FALSE;
      }
      else if (strbegw("gas", line))
      {
        clear_screen(BLACK, RED);
        gotoxy(1, 0);
        print_string("GAS");
        bool gasrunning = TRUE;
        for (uint16 port = 0x00; port < 255; port++)
        {
          if (!gasrunning)
          {
            break;
          }
          for (uint8 data = 0x00; data < 255; data++)
          {
            print_string(" port: ");
            print_int((int)port);
            print_string(" data: ");

            print_int((int)data);
            byte anss = get_input_keycode();
            if (anss == KEY_ENTER)
            {
              print_string(" data recv: ");
              outb(port, data);
              uint8 bata;
              bata = inb(port);
              //print_string("\"");
              print_int(bata);
            }
            else if (anss == KEY_SPACE)
            {
            }
            else if (anss == KEY_INSERT)
            {
              data = 255;
              port += 1;
            }
            else if (anss == KEY_END)
            {
              data = 255;
              port = 255;
              gasrunning = FALSE;
              clear_F();
              break;
            }
            else if (anss = KEY_HOME)
            {
              data = 255;
              port = 255;
              gasrunning = FALSE;
              clear_F();
              break;
            }

            sleep(2300000);
            sleep(29304890 * 5);
            sleep(39304890);
            sleep(39304890);
            //print_string("       ");

            //print_string("\"");
          }
        }
        //outb(0x01,0x01);
        while (strcomp("exit", line) != TRUE)
        {
          break;
          sleep(39304890);
          sleep(39304890);
          ans = get_input_keycode();
          //gotoxy(x, y);
          if ((int)(get_char(ans)) == 28)
          {
            break;
          }
          /*else if (get_char(ans) == '\0')
          {
          }*/
          else if (ans == KEY_ENTER)
          {
            if (strcomp(line, "exit"))
            {
              break;
            }
            //asm(line);
            print_new_line();
            int ia = 0;
            line[0] = '\0';
            l = 0;
          }
          else if (get_char(ans) == (char)8)
          {
            size = strlen(line);
            if (!size <= 1)
            {
              line[size - 1] = '\0';
              vga_index -= 1;
              l -= 1;
            }
          }
          else
          {
            line[l] = get_char(ans);
            l++;
            print_char(get_char(ans));

            //x++;
          }
        }
      }
      print_new_line();
      int ia = 0;
      line[0] = '\0';
      l = 0;
    }
    else if (get_char(ans) == '\0')
    {
    }
    else if (get_char(ans) == (char)8)
    {
      size = strlen(line);
      if (size > 0)
      {
        line[size - 1] = '\0';
        vga_index = vga_index - 1;
        l -= 1;
      }
    }
    else if (ans == KEY_HOME)
    {
      clear_F();
    }

    else
    {
      line[l] = get_char(ans);
      l++;
      print_char(get_char(ans));

      //x++;
    }
    if (BUFSIZE - 300 <= vga_index)
    {
      clear_F();
      gotoxy(0, 3);
    }

    //print_int(vga_index);
  }
}
