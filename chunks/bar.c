// draw the bar I suppose...
void drawbar(Monitor* m) {
  int          x, w, tw = 0, stw = 0;
  int          boxs = drw->fonts->h / 9;
  int          boxw = drw->fonts->h / 6 + 2;
  unsigned int i, occ = 0, urg = 0;
  Client*      c;

  if (showsystray && m == systraytomon(m) && !systrayonleft)
    stw = getsystraywidth();

  // draw status first so it can be overdrawn by tags later
  if (m == selmon || 1) { // draw status on every monitor
    tw = m->ww - drawstatusbar(m, bh, stext, stw);
  }

  // TODO: comment below
  resizebarwin(m);
  for (c = m->clients; c; c = c->next) {
    occ |= c->tags == 255 ? 0 : c->tags;
    if (c->isurgent)
      urg |= c->tags;
  }
  x = 0;
  for (i = 0; i < LENGTH(tags); i++) {
    // do not draw vacant tags
    if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
      continue;

    w = TEXTW(tags[i]);

    drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : m == selmon ? SchemeNorm
                                                                                       : SchemeDark]);
    drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
    x += w;
  }
  w = blw = TEXTW(m->ltsymbol);
  drw_setscheme(drw, scheme[m == selmon ? SchemeNorm : SchemeDark]);
  x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);

  if ((w = m->ww - tw - x) > bh) {
    if (m->sel) {
      drw_setscheme(drw, scheme[m == selmon ? SchemeSel : SchemeInv]);
      drw_text(drw, x, 0, w, bh, lrpad / 2, m->sel->name, 0);
      if (m->sel->isfloating)
        drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);
      if (m->sel->issticky)
        drw_polygon(drw, x + boxs, m->sel->isfloating ? boxs * 2 + boxw : boxs, stickyiconbb.x, stickyiconbb.y, boxw, boxw * stickyiconbb.y / stickyiconbb.x, stickyicon, LENGTH(stickyicon), Nonconvex, m->sel->tags & m->tagset[m->seltags]);
    } else {
      drw_setscheme(drw, scheme[m == selmon ? SchemeNorm : SchemeInv]);
      drw_rect(drw, x, 0, w, bh, 1, 1);
    }
  }
  drw_map(drw, m->barwin, 0, 0, m->ww, bh);
}

// draw all the bars on different monitors
void drawbars(void) {
  Monitor* m;

  for (m = mons; m; m = m->next)
    drawbar(m);
}

// update all the bars
void updatebars(void) {
  unsigned int         w;
  Monitor*             m;
  XSetWindowAttributes wa = {
    .override_redirect = True,
    .background_pixmap = ParentRelative,
    .event_mask        = ButtonPressMask | ExposureMask
  };
  XClassHint ch = { "dwm", "dwm" };
  for (m = mons; m; m = m->next) {
    if (m->barwin)
      continue;
    w = m->ww;
    if (showsystray && m == systraytomon(m))
      w -= getsystraywidth();
    m->barwin = XCreateWindow(dpy, root, m->wx, m->by, w, bh, 0, DefaultDepth(dpy, screen),
        CopyFromParent, DefaultVisual(dpy, screen),
        CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
    if (showsystray && m == systraytomon(m))
      XMapRaised(dpy, systray->win);
    XMapRaised(dpy, m->barwin);
    m->tabwin = XCreateWindow(dpy, root, m->wx, m->ty, m->ww, th, 0, DefaultDepth(dpy, screen),
        CopyFromParent, DefaultVisual(dpy, screen),
        CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    XDefineCursor(dpy, m->tabwin, cursor[CurNormal]->cursor);
    XMapRaised(dpy, m->tabwin);
    XSetClassHint(dpy, m->barwin, &ch);
  }
}

// draw all the tabs on different monitors
void drawtabs(void) {
  Monitor* m;

  for (m = mons; m; m = m->next)
    drawtab(m);
}

// compare int (idk really, it is needed for drawtab)
static int cmpint(const void* p1, const void* p2) {
  /* The actual arguments to this function are "pointers to
     pointers to char", but strcmp(3) arguments are "pointers
     to char", hence the following cast plus dereference */
  return *((int*)p1) > *(int*)p2;
}

void drawtab(Monitor* m) {
  Client*      c;
  unsigned int i;
  int          itag = -1;
  char         view_info[50];
  int          view_info_w = 0;
  int          sorted_label_widths[MAXTABS];
  int          tot_width;
  int          maxsize = bh;
  int          x       = 0;
  int          w       = 0;

  //view_info: indicate the tag which is displayed in the view
  for (i = 0; i < LENGTH(tags); ++i) {
    if ((selmon->tagset[selmon->seltags] >> i) & 1) {
      if (itag >= 0) { //more than one tag selected
        itag = -1;
        break;
      }
      itag = i;
    }
  }
  /* if (0 <= itag && itag < LENGTH(tags)) { */
  /*   snprintf(view_info, sizeof view_info, "[%s]", tags[itag]); */
  /* } else { */
  /*   strncpy(view_info, "[...]", sizeof view_info); */
  /* } */
  view_info[sizeof(view_info) - 1] = 0;
  view_info_w                      = TEXTW(view_info);
  tot_width                        = view_info_w;

  /* Calculates number of labels and their width */
  m->ntabs = 0;
  for (c = m->clients; c; c = c->next) {
    if (!ISVISIBLE(c))
      continue;
    m->tab_widths[m->ntabs] = TEXTW(c->name);
    tot_width += m->tab_widths[m->ntabs];
    ++m->ntabs;
    if (m->ntabs >= MAXTABS)
      break;
  }

  if (tot_width > m->ww) { //not enough space to display the labels, they need to be truncated
    memcpy(sorted_label_widths, m->tab_widths, sizeof(int) * m->ntabs);
    qsort(sorted_label_widths, m->ntabs, sizeof(int), cmpint);
    tot_width = view_info_w;
    for (i = 0; i < m->ntabs; ++i) {
      if (tot_width + (m->ntabs - i) * sorted_label_widths[i] > m->ww)
        break;
      tot_width += sorted_label_widths[i];
    }
    maxsize = (m->ww - tot_width) / (m->ntabs - i);
  } else {
    maxsize = m->ww;
  }
  i = 0;
  for (c = m->clients; c; c = c->next) {
    if (!ISVISIBLE(c))
      continue;
    if (i >= m->ntabs)
      break;
    if (m->tab_widths[i] > maxsize)
      m->tab_widths[i] = maxsize;
    w = m->tab_widths[i];
    drw_setscheme(drw, (c == m->sel) ? scheme[SchemeSel] : scheme[SchemeNorm]);
    drw_text(drw, x, 0, w, th, lrpad / 2, c->name, 0);
    x += w;
    ++i;
  }

  drw_setscheme(drw, scheme[SchemeNorm]);

  /* cleans interspace between window names and current viewed tag label */
  w = m->ww - view_info_w - x;
  drw_text(drw, x, 0, w, th, lrpad / 2, "", 0);

  /* view info */
  x += w;
  w = view_info_w;
  drw_text(drw, x, 0, w, th, lrpad / 2, view_info, 0);

  drw_map(drw, m->tabwin, 0, 0, m->ww, th);
}

// update position of the bar
void updatebarpos(Monitor* m) {
  Client* c;
  int     nvis = 0;

  m->wy = m->my;
  m->wh = m->mh;
  if (m->showbar) {
    m->wh -= bh;
    m->by = m->topbar ? m->wy : m->wy + m->wh;
    if (m->topbar)
      m->wy += bh;
  } else {
    m->by = -bh;
  }

  for (c = m->clients; c; c = c->next) {
    if (ISVISIBLE(c))
      ++nvis;
  }

  if (m->showtab == showtab_always
      || ((m->showtab == showtab_auto) && (nvis > 1) && (m->lt[m->sellt]->arrange == monocle))) {
    m->wh -= th;
    m->ty = m->toptab ? m->wy : m->wy + m->wh;
    if (m->toptab)
      m->wy += th;
  } else {
    m->ty = -th;
  }
}

// resize bar window
void resizebarwin(Monitor* m) {
  unsigned int w = m->ww;
  if (showsystray && m == systraytomon(m) && !systrayonleft)
    w -= getsystraywidth();
  XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, w, bh);
}

// apply systray to selected monitor
Monitor* systraytomon(Monitor* m) {
  Monitor* t;
  int      i, n;
  if (!systraypinning) {
    if (!m)
      return selmon;
    return m == selmon ? m : NULL;
  }
  for (n = 1, t = mons; t && t->next; n++, t = t->next)
    ;
  for (i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next)
    ;
  if (systraypinningfailfirst && n < systraypinning)
    return mons;
  return t;
}

// get width of systray
unsigned int getsystraywidth() {
  unsigned int w = 0;
  Client*      i;
  if (showsystray)
    for (i = systray->icons; i; w += i->w + systrayspacing, i = i->next)
      ;
  return w ? w + systrayspacing : 1;
}

// draw status bar (with pretty colors ^-^)
int drawstatusbar(Monitor* m, int bh, char* stext, int stw) {
  int   ret, i, w, x, len;
  short isCode = 0;
  char* text;
  char* p;
  Clr   oldbg, oldfg;

  // TODO: comment this stuff, refactor maybe

  len = strlen(stext) + 1;
  if (!(text = (char*)malloc(sizeof(char) * len)))
    die("malloc");
  p = text;
  memcpy(text, stext, len);

  /* compute width of the status text */
  w = 0;
  i = -1;
  while (text[++i]) {
    if (text[i] == '^') {
      if (!isCode) {
        isCode  = 1;
        text[i] = '\0';
        w += TEXTW(text) - lrpad;
        text[i] = '^';
        if (text[++i] == 'f')
          w += atoi(text + ++i);
      } else {
        isCode = 0;
        text   = text + i + 1;
        i      = -1;
      }
    }
  }
  if (!isCode)
    w += TEXTW(text) - lrpad;
  else
    isCode = 0;
  text = p;

  w += 2; /* 1px padding on both sides */
  ret = x = m->ww - w - stw;

  drw_setscheme(drw, scheme[LENGTH(colors)]);
  Clr *scheme_ = scheme[m == selmon ? SchemeNorm : SchemeDark];

  drw->scheme[ColFg] = scheme_[ColFg];
  drw->scheme[ColBg] = scheme_[ColBg];
  drw_rect(drw, x, 0, w, bh, 1, 1);
  x++;

  /* process status text */
  i = -1;
  while (text[++i]) {
    if (text[i] == '^' && !isCode) {
      isCode = 1;

      text[i] = '\0';
      w       = TEXTW(text) - lrpad;
      drw_text(drw, x, 0, w, bh, 0, text, 0);

      x += w;

      /* process code */
      while (text[++i] != '^') {
        if (text[i] == 'c') {
          char buf[8];
          memcpy(buf, (char*)text + i + 1, 7);
          buf[7] = '\0';
          drw_clr_create(drw, &drw->scheme[ColFg], buf);
          i += 7;
        } else if (text[i] == 'b') {
          char buf[8];
          memcpy(buf, (char*)text + i + 1, 7);
          buf[7] = '\0';
          drw_clr_create(drw, &drw->scheme[ColBg], buf);
          i += 7;
        } else if (text[i] == 'd') {
          drw->scheme[ColFg] = scheme_[ColFg];
          drw->scheme[ColBg] = scheme_[ColBg];
        } else if (text[i] == 'w') {
          Clr swp;
          swp                = drw->scheme[ColFg];
          drw->scheme[ColFg] = drw->scheme[ColBg];
          drw->scheme[ColBg] = swp;
        } else if (text[i] == 'v') {
          oldfg = drw->scheme[ColFg];
          oldbg = drw->scheme[ColBg];
        } else if (text[i] == 't') {
          drw->scheme[ColFg] = oldfg;
          drw->scheme[ColBg] = oldbg;
        } else if (text[i] == 'r') {
          int rx = atoi(text + ++i);
          while (text[++i] != ',')
            ;
          int ry = atoi(text + ++i);
          while (text[++i] != ',')
            ;
          int rw = atoi(text + ++i);
          while (text[++i] != ',')
            ;
          int rh = atoi(text + ++i);

          drw_rect(drw, rx + x, ry, rw, rh, 1, 0);
        } else if (text[i] == 'f') {
          x += atoi(text + ++i);
        }
      }

      text   = text + i + 1;
      i      = -1;
      isCode = 0;
    }
  }

  if (!isCode) {
    w = TEXTW(text) - lrpad;
    drw_text(drw, x, 0, w, bh, 0, text, 0);
  }

  drw_setscheme(drw, scheme[SchemeNorm]);
  free(p);

  return ret;
}

// update status in bar
void updatestatus(void) {
  Monitor* m;
  if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
    strcpy(stext, "dwm");
  for (m = mons; m; m = m->next)
    drawbar(m);
  updatesystray();
}

// update geometry of systray icons
void updatesystrayicongeom(Client* i, int w, int h) {
  if (i) {
    i->h = bh;
    if (w == h)
      i->w = bh;
    else if (h == bh)
      i->w = w;
    else
      i->w = (int)((float)bh * ((float)w / (float)h));
    applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
    // force icons into the systray dimensions if they don't want to
    if (i->h > bh) {
      if (i->w == i->h)
        i->w = bh;
      else
        i->w = (int)((float)bh * ((float)i->w / (float)i->h));
      i->h = bh;
    }
  }
}

// update states of systray icons
void updatesystrayiconstate(Client* i, XPropertyEvent* ev) {
  long flags;
  int  code = 0;

  // TODO: comment
  if (!showsystray || !i || ev->atom != xatom[XembedInfo] || !(flags = getatomprop(i, xatom[XembedInfo])))
    return;

  if (flags & XEMBED_MAPPED && !i->tags) {
    i->tags = 1;
    code    = XEMBED_WINDOW_ACTIVATE;
    XMapRaised(dpy, i->win);
    setclientstate(i, NormalState);
  } else if (!(flags & XEMBED_MAPPED) && i->tags) {
    i->tags = 0;
    code    = XEMBED_WINDOW_DEACTIVATE;
    XUnmapWindow(dpy, i->win);
    setclientstate(i, WithdrawnState);
  } else
    return;
  sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
      systray->win, XEMBED_EMBEDDED_VERSION);
}

// update systray
void updatesystray(void) {
  XSetWindowAttributes wa;
  XWindowChanges       wc;
  Client*              i;
  Monitor*             m  = systraytomon(NULL);
  unsigned int         x  = m->mx + m->mw;
  unsigned int         sw = TEXTW(stext) - lrpad + systrayspacing;
  unsigned int         w  = 1;

  // TODO: comment
  if (!showsystray)
    return;
  if (systrayonleft)
    x -= sw;
  if (!systray) {
    /* init systray */
    if (!(systray = (Systray*)calloc(1, sizeof(Systray))))
      die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
    systray->win         = XCreateSimpleWindow(dpy, root, x, m->by, w, bh, 0, 0, scheme[SchemeSel][ColBg].pixel);
    wa.event_mask        = ButtonPressMask | ExposureMask;
    wa.override_redirect = True;
    wa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
    XSelectInput(dpy, systray->win, SubstructureNotifyMask);
    XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
        PropModeReplace, (unsigned char*)&netatom[NetSystemTrayOrientationHorz], 1);
    XChangeWindowAttributes(dpy, systray->win, CWEventMask | CWOverrideRedirect | CWBackPixel, &wa);
    XMapRaised(dpy, systray->win);
    XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
    if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
      sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
      XSync(dpy, False);
    } else {
      fprintf(stderr, "dwm: unable to obtain system tray.\n");
      free(systray);
      systray = NULL;
      return;
    }
  }
  for (w = 0, i = systray->icons; i; i = i->next) {
    /* make sure the background color stays the same */
    wa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
    XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
    XMapRaised(dpy, i->win);
    w += systrayspacing;
    i->x = w;
    XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
    w += i->w;
    if (i->mon != m)
      i->mon = m;
  }
  w = w ? w + systrayspacing : 1;
  x -= w;
  XMoveResizeWindow(dpy, systray->win, x, m->by, w, bh);
  wc.x          = x;
  wc.y          = m->by;
  wc.width      = w;
  wc.height     = bh;
  wc.stack_mode = Above;
  wc.sibling    = m->barwin;
  XConfigureWindow(dpy, systray->win, CWX | CWY | CWWidth | CWHeight | CWSibling | CWStackMode, &wc);
  XMapWindow(dpy, systray->win);
  XMapSubwindows(dpy, systray->win);
  /* redraw background */
  XSetForeground(dpy, drw->gc, scheme[SchemeNorm][ColBg].pixel);
  XFillRectangle(dpy, systray->win, drw->gc, 0, 0, w, bh);
  XSync(dpy, False);
}

// TODO: figure out wtf is this
Client* wintosystrayicon(Window w) {
  Client* i = NULL;

  if (!showsystray || !w)
    return i;
  for (i = systray->icons; i && i->win != w; i = i->next)
    ;
  return i;
}

// remove system tray icon
void removesystrayicon(Client* i) {
  Client** ii;

  if (!showsystray || !i)
    return;
  for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next)
    ;
  if (ii)
    *ii = i->next;
  free(i);
}
