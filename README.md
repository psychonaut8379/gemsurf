# Gemsurf

![demo](https://i.imgur.com/WUhlXMI.gif)

A lightweight TUI Gemini client written in C.

## Prerequisites 

- C compiler
- OpenSSL
- ncurses

## Installation 

```
git clone https://github.com/psychonaut8379/Gemsurf.git
make && make install
gemsurf
```

## Controls

| Key | Action | 
|:---:|:-------|
| Tab | Switch to Selection mode |
| ESC | switch back to view mode |
| Enter | go to the link you selected |
| j | scroll down or go to the next link (in selection mode) |
| k | scroll up or go to the previous link (in selection mode) |
| n | next page |
| b | previous page |
| / | search bar |
| q | quit from the program |

## Roadmap
- [x] **TOFU** - Gemsurf supports TOFU model experimentally
- [x] **Preformatted blocks** - preformatted lines should not get formatted at all, they should not wrap to new lines. so normally this behaviour can be achieved with horizontal scrollbars. but implementing this would be extremely hard on terminal environment so currently gemsurf just cuts the line if it is wider than the screen
- [x] **Extended protocol support** - From now on gemsurf prompts for comfirmation to open link in external app if it can't handle url
- [ ] **Improved UI** - Current UI design is just a temporary placeholder i will improve the UI
- [ ] **Folder View** - Gemsurf can only open files currently but it can open folders in future