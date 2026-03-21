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
make
./gemsurf
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
- [ ] **TOFU** - Without tofu the program isn't functional yet so i will implement TOFU in first place
- [ ] **Preformatted blocks** - Currently Gemsurf doesn't support preformatted blocks. But since this is a core part of the gemtext i will add support as soon as
- [x] **Extended protocol support** - From now on gemsurf prompts for comfirmation to open link in external app if it can't handle url
- [ ] **Improved UI** - Current UI design is just a temporary placeholder i will improve the UI
- [ ] **Folder View** - Gemsurf can only open files currently but it can open folders in future