#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>
#include <string.h>
#include <ncurses.h>
#include <ctype.h>
#include <strings.h>



#define PERMISOS 0644

/* Acciones */
#define LOGIN 1
#define REGISTRO 2
#define VER_PRODUCTOS 3
#define AGREGAR_CARRITO 4
#define VER_CARRITO 5
#define COMPRAR_CARRITO 6
#define VER_HISTORIAL 7
#define BUSCAR_PRODUCTO 8
#define VER_USUARIOS 9
#define CREAR_USUARIO_ADMIN 10
#define MODIFICAR_USUARIO 11
#define ELIMINAR_USUARIO 12
#define REPORTE_DIARIO 13
#define REPORTE_SEMANAL 14
#define REPORTE_MENSUAL 15
#define AGREGAR_PRODUCTO_ADMIN 16
#define MODIFICAR_STOCK 17
#define ELIMINAR_PRODUCTO 18
#define MODIFICAR_MI_CUENTA 19

/* Colores */
#define COLOR_TITULO   1
#define COLOR_NORMAL   2
#define COLOR_SELEC    3
#define COLOR_ERROR    4
#define COLOR_OK       5
#define COLOR_BORDE    6

/* ── Estructuras ── */
struct Productos {
    int   id;
    char  nombre[50];
    char  plataforma[50];
    float precio;
    int   stock;
};

struct Datos {
    int  accion;
    int  es_admin;
    char usuario[50];
    char password[100];
    char correo[70];
    char respuesta[50];
    char mensaje[200];
    char productos[1500];
    struct Productos producto;
};

struct Datos *memoria;

/* ── Prototipos de funciones ── */
void inicializar_colores(void);
void up(int semid);
void down(int semid);
void dibujar_ventana(WINDOW *w, const char *titulo);
void popup_mensaje(const char *titulo, const char *mensaje, int es_error);
void mostrar_mensaje(const char *msg, int es_error);
void esperar_enter(void);
int leer_cadena(WINDOW *w, int fila, int col, char *buf, int max);
int leer_password(WINDOW *w, int fila, int col, char *buf, int max);
void dibujar_leyenda(const char *texto);
int menu_navegar(WINDOW *w, const char **opciones, int n, const char *titulo);
void banner(void);
int pantalla_menu_inicial(void);
void pantalla_login(char *usuario, char *password, char *correo, int es_registro);
int pantalla_menu_usuario(const char *nombre);
int pantalla_menu_admin(void);
int pantalla_catalogo(struct Productos *lista, int total, int *cantidad_out);
int pantalla_carrito(struct Productos *carrito, int total, float costo_total);
void pantalla_texto(const char *titulo, const char *texto);
int pantalla_formulario(const char *titulo, const char **campos, char **valores,
                        const int *maxlen, const int *es_pass, int n_campos);

/* ── Implementación de funciones ── */

void inicializar_colores(void) {
    start_color();
    init_pair(COLOR_TITULO, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_NORMAL, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_SELEC,  COLOR_BLACK, COLOR_GREEN);
    init_pair(COLOR_ERROR,  COLOR_RED,   COLOR_BLACK);
    init_pair(COLOR_OK,     COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_BORDE,  COLOR_GREEN, COLOR_BLACK);
}

void up(int semid) {
    struct sembuf op = {0, +1, 0};
    semop(semid, &op, 1);
}

void down(int semid) {
    struct sembuf op = {0, -1, 0};
    semop(semid, &op, 1);
}

void dibujar_ventana(WINDOW *w, const char *titulo) {
    wattron(w, COLOR_PAIR(COLOR_BORDE) | A_BOLD);
    box(w, 0, 0);
    wattroff(w, COLOR_PAIR(COLOR_BORDE) | A_BOLD);

    if (titulo) {
        int ancho = getmaxx(w);
        int pos   = (ancho - (int)strlen(titulo) - 4) / 2;
        if (pos < 1) pos = 1;
        wattron(w, COLOR_PAIR(COLOR_TITULO) | A_BOLD);
        mvwprintw(w, 0, pos, "[ %s ]", titulo);
        wattroff(w, COLOR_PAIR(COLOR_TITULO) | A_BOLD);
    }
    wrefresh(w);
}

void popup_mensaje(const char *titulo, const char *mensaje, int es_error) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    int alto  = 9;
    int ancho = 60;
    int y = (filas - alto) / 2;
    int x = (cols  - ancho) / 2;

    WINDOW *popup = newwin(alto, ancho, y, x);
    keypad(popup, TRUE);
    dibujar_ventana(popup, titulo);

    if (es_error)
        wattron(popup, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    else
        wattron(popup, COLOR_PAIR(COLOR_OK) | A_BOLD);

    mvwprintw(popup, 3, (ancho - (int)strlen(mensaje)) / 2, "%s", mensaje);

    if (es_error)
        wattroff(popup, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    else
        wattroff(popup, COLOR_PAIR(COLOR_OK) | A_BOLD);

    wattron(popup, COLOR_PAIR(COLOR_NORMAL) | A_DIM);
    mvwprintw(popup, 6, (ancho - 26) / 2, "[ ENTER PARA CONTINUAR ]");
    wattroff(popup, COLOR_PAIR(COLOR_NORMAL) | A_DIM);

    wrefresh(popup);

    int ch;
    while ((ch = wgetch(popup)) != '\n' && ch != KEY_ENTER) {
        /* Esperar ENTER */
    }

    werase(popup);
    wrefresh(popup);
    delwin(popup);
}

void mostrar_mensaje(const char *msg, int es_error) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);
    (void)cols; /* Suprimir warning de variable no usada */
    
    move(filas - 2, 0);
    clrtoeol();
    
    if (es_error)
        attron(COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    else
        attron(COLOR_PAIR(COLOR_OK) | A_BOLD);
    
    mvprintw(filas - 2, 2, "[SYS] %s", msg);
    
    if (es_error)
        attroff(COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    else
        attroff(COLOR_PAIR(COLOR_OK) | A_BOLD);
    
    refresh();
}

void esperar_enter(void) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);
    (void)cols;
    
    attron(COLOR_PAIR(COLOR_NORMAL) | A_DIM);
    mvprintw(filas - 1, 2, "> PRESS [ENTER] TO CONTINUE...");
    attroff(COLOR_PAIR(COLOR_NORMAL) | A_DIM);
    refresh();
    
    int c;
    while ((c = getch()) != '\n' && c != KEY_ENTER) {
        /* Esperar */
    }
}

int leer_cadena(WINDOW *w, int fila, int col, char *buf, int max) {
    echo();
    curs_set(1);
    wmove(w, fila, col);
    wclrtoeol(w);
    wattron(w, COLOR_PAIR(COLOR_NORMAL));
    wgetnstr(w, buf, max - 1);
    wattroff(w, COLOR_PAIR(COLOR_NORMAL));
    noecho();
    curs_set(0);

    buf[strcspn(buf, "\r\n")] = 0;
    if (strcasecmp(buf, "q") == 0) return 0;
    return 1;
}

int leer_password(WINDOW *w, int fila, int col, char *buf, int max) {
    int ch, pos = 0;
    buf[0] = '\0';
    wmove(w, fila, col);
    wclrtoeol(w);
    
    noecho();
    curs_set(1);

    while (1) {
        ch = wgetch(w);
        
        if (ch == 'q' || ch == 'Q') return 0;
        if (ch == '\n') break;
        
        if ((ch == KEY_BACKSPACE || ch == 127) && pos > 0) {
            pos--;
            buf[pos] = '\0';
            mvwprintw(w, fila, col + pos, " ");
            wmove(w, fila, col + pos);
        }
        else if (pos < max - 1 && isprint(ch)) {
            buf[pos++] = ch;
            buf[pos] = '\0';
            mvwaddch(w, fila, col + pos - 1, '#');
        }
    }
    
    curs_set(0);
    return 1;
}

void dibujar_leyenda(const char *texto) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    move(max_y - 1, 0);
    clrtoeol();

    attron(COLOR_PAIR(COLOR_TITULO) | A_DIM);
    mvprintw(max_y - 1, (max_x - (int)strlen(texto)) / 2, "%s", texto);
    attroff(COLOR_PAIR(COLOR_TITULO) | A_DIM);
    
    refresh();
}

int menu_navegar(WINDOW *w, const char **opciones, int n, const char *titulo) {
    int selec = 0;
    int tecla;
    int max_y, max_x;
    
    getmaxyx(stdscr, max_y, max_x);

    while (1) {
        dibujar_ventana(w, titulo);

        for (int i = 0; i < n; i++) {
            if (i == selec) {
                wattron(w, COLOR_PAIR(COLOR_SELEC) | A_BOLD);
                mvwprintw(w, 2 + i, 2, " > %s ", opciones[i]);
                wattroff(w, COLOR_PAIR(COLOR_SELEC) | A_BOLD);
            } else {
                wattron(w, COLOR_PAIR(COLOR_NORMAL) | A_BOLD);
                mvwprintw(w, 2 + i, 2, "   %s ", opciones[i]);
                wattroff(w, COLOR_PAIR(COLOR_NORMAL) | A_BOLD);
            }
        }
        wrefresh(w);

        move(max_y - 1, 0);
        clrtoeol();
        
        attron(COLOR_PAIR(COLOR_TITULO) | A_DIM);
        const char *texto = "ENTER: Seleccionar | FLECHAS: Navegar | ESC: Salir";
        mvprintw(max_y - 1, (max_x - (int)strlen(texto)) / 2, "%s", texto);
        attroff(COLOR_PAIR(COLOR_TITULO) | A_DIM);
        
        refresh();

        tecla = wgetch(w);

        if (tecla == KEY_RESIZE) {
            getmaxyx(stdscr, max_y, max_x);
            return KEY_RESIZE;
        }
        else if (tecla == KEY_UP) selec = (selec - 1 + n) % n;
        else if (tecla == KEY_DOWN) selec = (selec + 1) % n;
        else if (tecla == 10 || tecla == KEY_ENTER) return selec;
        else if (tecla == 27) return -1;
    }
}

void banner(void) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    (void)max_y;

    attron(COLOR_PAIR(COLOR_TITULO) | A_BOLD);

    mvprintw(2, (max_x - 50) / 2, "  ____    _    __  __  _____ ____  _   _ __  __ ");
    mvprintw(3, (max_x - 50) / 2, " / ___|  / \\  |  \\/  || ____| __ )| | | |\\ \\/ / ");
    mvprintw(4, (max_x - 50) / 2, "| |  _  / _ \\ | |\\/| ||  _| |  _ \\| | | | \\  /  ");
    mvprintw(5, (max_x - 50) / 2, "| |_| |/ ___ \\| |  | || |___| |_) | |_| | / /   ");
    mvprintw(6, (max_x - 50) / 2, " \\____/_/   \\_\\_|  |_||_____|____/ \\___/ /_/    ");
    
    mvprintw(8, 4, "<");
    mvhline(8, 5, '-', max_x - 10);
    mvprintw(8, max_x - 5, ">");
    
    attroff(COLOR_PAIR(COLOR_TITULO) | A_BOLD);
    refresh();
}

int pantalla_menu_inicial(void) {
    int r = KEY_RESIZE;
    int filas, cols;
    int alto = 10, ancho = 40;

    while (r == KEY_RESIZE) {
        clear();
        banner();
        getmaxyx(stdscr, filas, cols);

        int fin_banner = 9;
        int espacio_libre = filas - fin_banner;
        int y0 = fin_banner + (espacio_libre - alto) / 2;
        int x0 = (cols - ancho) / 2;

        WINDOW *w = newwin(alto, ancho, y0, x0);
        keypad(w, TRUE);

        const char *opciones[] = {
            "[LOG] INICIAR SESION",
            "[REG] REGISTRARSE",
            "[EXIT] SALIR DEL SISTEMA"
        };
        
        r = menu_navegar(w, opciones, 3, "VIDEOJUEGOS");
        
        delwin(w);
    }
    return r;
}

void pantalla_login(char *usuario, char *password, char *correo, int es_registro) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    int alto = es_registro ? 14 : 12;
    int ancho = 50;
    int y0 = (filas - alto) / 2;
    int x0 = (cols - ancho) / 2;

    WINDOW *w = newwin(alto, ancho, y0, x0);
    keypad(w, TRUE);
    noecho();
    curs_set(1);

    const char *titulo = es_registro ? "REGISTRO" : "LOGIN";
    
    int total_campos = es_registro ? 4 : 3;
    int campo_actual = 0;
    int tecla = 0;

    char mensaje_error[60] = "";

    memset(usuario, 0, 50);
    memset(password, 0, 100);
    correo[0] = '\0';

    flushinp();

    while (1) {
        werase(w);
        dibujar_ventana(w, titulo);

        /* Campo 1: CORREO (Login) o NOMBRE (Registro) */
        if (campo_actual == 0)
            wattron(w, COLOR_PAIR(COLOR_SELEC) | A_BOLD);
        else
            wattron(w, COLOR_PAIR(COLOR_TITULO));
        
        if (es_registro) {
            mvwprintw(w, 2, 2, "NOMBRE > %s", usuario);
        } else {
            mvwprintw(w, 2, 2, "CORREO > %s", correo);
        }
        wattroff(w, COLOR_PAIR(COLOR_SELEC) | COLOR_PAIR(COLOR_TITULO) | A_BOLD);

        /* Campo 2: CONTRASENA */
        if (campo_actual == 1)
            wattron(w, COLOR_PAIR(COLOR_SELEC) | A_BOLD);
        else
            wattron(w, COLOR_PAIR(COLOR_TITULO));
        
        char asteriscos[100];
        size_t pass_len = strlen(password);
        memset(asteriscos, '*', pass_len);
        asteriscos[pass_len] = '\0';

        mvwprintw(w, 5, 2, "CONTRASENA > %s", asteriscos);
        wattroff(w, COLOR_PAIR(COLOR_SELEC) | COLOR_PAIR(COLOR_TITULO) | A_BOLD);

        /* Requisitos de contraseña (solo registro) */
        int tiene_mayus = 0, tiene_minus = 0, tiene_num = 0, tiene_esp = 0;
        if (es_registro) {
            for (size_t i = 0; i < pass_len; i++) {
                if (isupper((unsigned char)password[i])) tiene_mayus = 1;
                else if (islower((unsigned char)password[i])) tiene_minus = 1;
                else if (isdigit((unsigned char)password[i])) tiene_num = 1;
                else if (ispunct((unsigned char)password[i]) || password[i] == ' ') tiene_esp = 1;
            }

            mvwprintw(w, 7, 4, "Falta:");
            wattron(w, COLOR_PAIR(tiene_mayus ? COLOR_SELEC : COLOR_TITULO));
            mvwprintw(w, 7, 11, "[%sA]", tiene_mayus ? "X" : " ");
            wattron(w, COLOR_PAIR(tiene_minus ? COLOR_SELEC : COLOR_TITULO));
            mvwprintw(w, 7, 17, "[%sa]", tiene_minus ? "X" : " ");
            wattron(w, COLOR_PAIR(tiene_num ? COLOR_SELEC : COLOR_TITULO));
            mvwprintw(w, 7, 23, "[%s1]", tiene_num ? "X" : " ");
            wattron(w, COLOR_PAIR(tiene_esp ? COLOR_SELEC : COLOR_TITULO));
            mvwprintw(w, 7, 29, "[%s*]", tiene_esp ? "X" : " ");
            wattroff(w, COLOR_PAIR(COLOR_SELEC) | COLOR_PAIR(COLOR_TITULO));
        }

        /* Campo 3: CORREO (solo registro) */
        if (es_registro) {
            if (campo_actual == 2)
                wattron(w, COLOR_PAIR(COLOR_SELEC) | A_BOLD);
            else
                wattron(w, COLOR_PAIR(COLOR_TITULO));
            mvwprintw(w, 9, 2, "CORREO > %s", correo);
            wattroff(w, COLOR_PAIR(COLOR_SELEC) | COLOR_PAIR(COLOR_TITULO) | A_BOLD);
        }

        /* Mensaje de error */
        if (mensaje_error[0] != '\0') {
            wattron(w, COLOR_PAIR(COLOR_ERROR) | A_BLINK | A_BOLD);
            mvwprintw(w, 11, 2, "%s", mensaje_error);
            wattroff(w, COLOR_PAIR(COLOR_ERROR) | A_BLINK | A_BOLD);
        }

        /* Botón CONFIRMAR */
        int boton_y = es_registro ? 12 : 8;
        if (campo_actual == (total_campos - 1)) {
            wattron(w, COLOR_PAIR(COLOR_SELEC) | A_REVERSE | A_BOLD);
        } else {
            wattron(w, COLOR_PAIR(COLOR_NORMAL));
        }
        mvwprintw(w, boton_y, (ancho - 14) / 2, "[ CONFIRMAR ]");
        wattroff(w, COLOR_PAIR(COLOR_SELEC) | COLOR_PAIR(COLOR_NORMAL) | A_REVERSE | A_BOLD);

        /* Posición del cursor */
        if (campo_actual == 0) {
            wmove(w, 2, 2 + 9 + (int)strlen(es_registro ? usuario : correo));
        } else if (campo_actual == 1) {
            wmove(w, 5, 2 + 13 + (int)strlen(password));
        } else if (campo_actual == 2 && es_registro) {
            wmove(w, 9, 2 + 9 + (int)strlen(correo));
        } else if (campo_actual == total_campos - 1) {
            curs_set(0);
        }

        if (campo_actual != total_campos - 1)
            curs_set(1);

        wrefresh(w);

        tecla = wgetch(w);

        if (tecla >= 32 && tecla <= 126) {
            mensaje_error[0] = '\0';
        }

        /* Validación del correo */
        size_t len_correo = strlen(correo);
        int correo_valido = 0;
        
        if (!es_registro && strcmp(correo, "admin") == 0) {
            correo_valido = 1;
        } else if (len_correo >= 4 && strcmp(correo + len_correo - 4, ".com") == 0) {
            correo_valido = 1;
        }

        /* Navegación */
        if (tecla == KEY_UP) {
            if ((es_registro && campo_actual == 2 && !correo_valido) ||
                (!es_registro && campo_actual == 0 && !correo_valido && len_correo > 0)) {
                strcpy(mensaje_error, "! EL CORREO DEBE TERMINAR EN .com !");
                beep();
            } else {
                campo_actual = (campo_actual - 1 + total_campos) % total_campos;
            }
        }
        else if (tecla == KEY_DOWN || tecla == 9) {
            if ((es_registro && campo_actual == 2 && !correo_valido) ||
                (!es_registro && campo_actual == 0 && !correo_valido && len_correo > 0)) {
                strcpy(mensaje_error, "! EL CORREO DEBE TERMINAR EN .com !");
                beep();
            } else {
                campo_actual = (campo_actual + 1) % total_campos;
            }
        }
        else if (tecla == 10 || tecla == KEY_ENTER) {
            if ((es_registro && campo_actual == 2 && !correo_valido) ||
                (!es_registro && campo_actual == 0 && !correo_valido)) {
                strcpy(mensaje_error, "! EL CORREO DEBE TERMINAR EN .com !");
                beep();
            }
            else if (campo_actual == total_campos - 1) {
                if (!correo_valido) {
                    strcpy(mensaje_error, "! EL CORREO DEBE TERMINAR EN .com !");
                    campo_actual = es_registro ? 2 : 0;
                    beep();
                }
                else if (es_registro && (!tiene_mayus || !tiene_minus || !tiene_num || !tiene_esp)) {
                    strcpy(mensaje_error, "! LA CONTRASENA NO CUMPLE REQUISITOS !");
                    campo_actual = 1;
                    beep();
                } else {
                    break;
                }
            } else {
                campo_actual = (campo_actual + 1) % total_campos;
            }
        }
        else if (tecla == 27) {
            usuario[0] = '\0';
            password[0] = '\0';
            correo[0] = '\0';
            break;
        }
        else if (campo_actual < (total_campos - 1)) {
            char *cadena_activa = NULL;
            int max_longitud = 0;

            if (campo_actual == 0) {
                cadena_activa = es_registro ? usuario : correo;
                max_longitud = es_registro ? 49 : 69;
            }
            else if (campo_actual == 1) {
                cadena_activa = password;
                max_longitud = 99;
            }
            else if (campo_actual == 2 && es_registro) {
                cadena_activa = correo;
                max_longitud = 69;
            }

            if (cadena_activa != NULL) {
                int len = (int)strlen(cadena_activa);
                
                if (tecla == KEY_BACKSPACE || tecla == 127 || tecla == 8) {
                    if (len > 0) {
                        cadena_activa[len - 1] = '\0';
                    }
                }
                else if (tecla >= 32 && tecla <= 126 && len < max_longitud) {
                    cadena_activa[len] = (char)tecla;
                    cadena_activa[len + 1] = '\0';
                }
            }
        }
    }

    curs_set(0);
    delwin(w);
    clear();
    refresh();
}

int pantalla_menu_usuario(const char *nombre) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    attron(COLOR_PAIR(COLOR_TITULO) | A_BOLD);
    mvprintw(1, (cols - 30) / 2, ">> BIENVENID@: %s <<", nombre);
    attroff(COLOR_PAIR(COLOR_TITULO) | A_BOLD);
    refresh();

    int alto = 14, ancho = 40;
    int y0 = (filas - alto) / 2;
    int x0 = (cols - ancho) / 2;

    WINDOW *w = newwin(alto, ancho, y0, x0);
    keypad(w, TRUE);

    const char *opciones[] = {
        "Ver catalogo",
        "Ver carrito",
        "Historial de compras",
        "Buscar producto",
        "Modificar mi cuenta",
        "Cerrar sesion"
    };
    int r = menu_navegar(w, opciones, 6, "MENU USUARIO");
    delwin(w);
    clear();
    refresh();
    return r;
}

int pantalla_menu_admin(void) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    int alto = 20, ancho = 44;
    int y0 = (filas - alto) / 2;
    int x0 = (cols - ancho) / 2;

    WINDOW *w = newwin(alto, ancho, y0, x0);
    keypad(w, TRUE);

    const char *opciones[] = {
        "Ver catalogo",
        "Agregar producto",
        "Modificar stock",
        "Eliminar producto",
        "Buscar producto",
        "Ver usuarios",
        "Crear usuario",
        "Modificar usuario",
        "Eliminar usuario",
        "Reporte ventas diarias",
        "Reporte ventas semanales",
        "Reporte ventas mensuales",
        "Cerrar sesion"
    };
    int r = menu_navegar(w, opciones, 13, "MENU ADMIN");
    delwin(w);
    clear();
    refresh();
    return r;
}

int pantalla_catalogo(struct Productos *lista, int total, int *cantidad_out) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    int alto = total + 7, ancho = 72;
    if (alto > filas - 4) alto = filas - 4;
    int y0 = (filas - alto) / 2;
    int x0 = (cols - ancho) / 2;

    WINDOW *w = newwin(alto, ancho, y0, x0);
    keypad(w, TRUE);
    dibujar_ventana(w, "CATALOGO");

    wattron(w, COLOR_PAIR(COLOR_BORDE) | A_BOLD);
    mvwprintw(w, 1, 2, "ID  %-28s %-14s  PRECIO    DISPONIBLE", "NOMBRE", "PLATAFORMA");
    wattroff(w, COLOR_PAIR(COLOR_BORDE) | A_BOLD);

    int selec = 0;
    while (1) {
        for (int i = 0; i < total; i++) {
            if (i == selec)
                wattron(w, COLOR_PAIR(COLOR_SELEC) | A_BOLD);
            else
                wattron(w, COLOR_PAIR(COLOR_NORMAL));

            mvwprintw(w, 2 + i, 2, "%-3d %-28.28s %-14.14s $%7.2f  %3d",
                      lista[i].id, lista[i].nombre, lista[i].plataforma,
                      lista[i].precio, lista[i].stock);

            if (i == selec)
                wattroff(w, COLOR_PAIR(COLOR_SELEC) | A_BOLD);
            else
                wattroff(w, COLOR_PAIR(COLOR_NORMAL));
        }
        dibujar_leyenda("ENTER: Seleccionar | FLECHAS: Navegar | ESC: Salir");
        wrefresh(w);

        int tecla = wgetch(w);
        if (tecla == KEY_UP)
            selec = (selec - 1 + total) % total;
        else if (tecla == KEY_DOWN)
            selec = (selec + 1) % total;
        else if (tecla == 'q' || tecla == 27) {
            delwin(w);
            clear();
            refresh();
            return -1;
        }
        else if (tecla == '\n' || tecla == KEY_ENTER) {
            WINDOW *wc = newwin(7, 44, y0 + alto/2, x0 + 14);
            keypad(wc, TRUE);
            dibujar_ventana(wc, "TRANSACCION");
            wattron(wc, COLOR_PAIR(COLOR_TITULO));
            mvwprintw(wc, 2, 2, "> Stock Disponible: %d", lista[selec].stock);
            mvwprintw(wc, 3, 2, "> Cantidad : ");
            wattroff(wc, COLOR_PAIR(COLOR_TITULO));

            char buf[10] = {0};
            leer_cadena(wc, 3, 21, buf, 9);
            int cant = atoi(buf);
            delwin(wc);

            if (cant <= 0) {
                popup_mensaje("ERROR", "La cantidad debe ser mayor a cero", 1);
                touchwin(w);
                wrefresh(w);
            }
            else if (cant > lista[selec].stock) {
                popup_mensaje("ERROR", "No hay suficiente stock disponible", 1);
                touchwin(w);
                wrefresh(w);
            }
            else {
                *cantidad_out = cant;
                delwin(w);
                clear();
                refresh();
                return selec;
            }
        }
    }
}

int pantalla_carrito(struct Productos *carrito, int total, float costo_total) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    int alto = total + 9, ancho = 68;
    if (alto > filas - 4) alto = filas - 4;
    int y0 = (filas - alto) / 2;
    int x0 = (cols - ancho) / 2;

    WINDOW *w = newwin(alto, ancho, y0, x0);
    keypad(w, TRUE);
    dibujar_ventana(w, "CARRITO DE COMPRAS");

    wattron(w, COLOR_PAIR(COLOR_BORDE) | A_BOLD);
    mvwprintw(w, 1, 2, "ID  %-28s %-14s  PRECIO", "NOMBRE", "PLATAFORMA");
    wattroff(w, COLOR_PAIR(COLOR_BORDE) | A_BOLD);

    for (int i = 0; i < total; i++) {
        wattron(w, COLOR_PAIR(COLOR_NORMAL));
        mvwprintw(w, 2 + i, 2, "%-3d %-28.28s %-14.14s $%7.2f",
                  carrito[i].id, carrito[i].nombre, carrito[i].plataforma, carrito[i].precio);
        wattroff(w, COLOR_PAIR(COLOR_NORMAL));
    }

    wrefresh(w);

    wattron(w, COLOR_PAIR(COLOR_OK) | A_BOLD);
    mvwprintw(w, 2 + total + 1, 2, "TOTAL_DUE: $%.2f", costo_total);
    wattroff(w, COLOR_PAIR(COLOR_OK) | A_BOLD);

    WINDOW *wm = newwin(7, 36, y0 + alto/2, x0 + 16);
    keypad(wm, TRUE);
    const char *opts[] = {"Aceptar", "Cancelar"};
    int r = menu_navegar(wm, opts, 2, "Continuar?");
    delwin(wm);
    delwin(w);
    clear();
    refresh();
    return (r == 0) ? 1 : 0;
}

void pantalla_texto(const char *titulo, const char *texto) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    int alto = filas - 4, ancho = cols - 4;
    WINDOW *w = newwin(alto, ancho, 2, 2);
    keypad(w, TRUE);
    dibujar_ventana(w, titulo);

    char copia[4096];
    strncpy(copia, texto, sizeof(copia) - 1);
    copia[sizeof(copia) - 1] = '\0';

    char *linea = strtok(copia, "\n");
    int fila_actual = 2;

    while (linea != NULL) {
        if (fila_actual >= alto - 2) {
            wattroff(w, COLOR_PAIR(COLOR_NORMAL));
            mvwprintw(w, alto - 2, 2, "-- Más abajo --");
            wrefresh(w);
            wgetch(w);
            wclear(w);
            dibujar_ventana(w, titulo);
            fila_actual = 2;
        }

        wattron(w, COLOR_PAIR(COLOR_NORMAL));
        mvwprintw(w, fila_actual++, 2, "%s", linea);
        wattroff(w, COLOR_PAIR(COLOR_NORMAL));
        
        linea = strtok(NULL, "\n");
    }

    wattron(w, COLOR_PAIR(COLOR_NORMAL));
    mvwprintw(w, alto - 1, 2, "Presione cualquier tecla para regresar...");
    wattroff(w, COLOR_PAIR(COLOR_NORMAL));
    
    wrefresh(w);
    wgetch(w);
    delwin(w);
    clear();
    refresh();
}

int pantalla_formulario(const char *titulo, const char **campos, char **valores,
                        const int *maxlen, const int *es_pass, int n_campos)
{
    int filas, cols;
    getmaxyx(stdscr, filas, cols);
    int alto = n_campos * 3 + 5, ancho = 52;
    WINDOW *w = newwin(alto, ancho, (filas - alto) / 2, (cols - ancho) / 2);
    keypad(w, TRUE);
    dibujar_ventana(w, titulo);

    int exito = 1;
    for (int i = 0; i < n_campos; i++) {
        wattron(w, COLOR_PAIR(COLOR_TITULO));
        mvwprintw(w, 2 + i * 3, 2, "> %s:", campos[i]);
        wattroff(w, COLOR_PAIR(COLOR_TITULO));
        
        wattron(w, A_DIM);
        mvwprintw(w, 2 + i * 3, 2 + (int)strlen(campos[i]) + 4, "[q + Enter para salir]");
        wattroff(w, A_DIM);

        if (es_pass[i])
            exito = leer_password(w, 3 + i * 3, 2, valores[i], maxlen[i]);
        else
            exito = leer_cadena(w, 3 + i * 3, 2, valores[i], maxlen[i]);
        
        if (!exito) break;
    }

    delwin(w);
    clear();
    refresh();
    return exito;
}

/* ══════════════════════════════════════════════════════════════════
   MAIN
   ══════════════════════════════════════════════════════════════════ */

int main(void) {
    /* Semáforos y memoria compartida */
    key_t llave = ftok("archivo", 'k');
    key_t llave_cliente = ftok("archivo", 'c');

    if (llave == -1 || llave_cliente == -1) {
        fprintf(stderr, "Error al crear la llave\n");
        exit(1);
    }

    int semaforo = semget(llave, 1, PERMISOS);
    int semaforo_cliente = semget(llave_cliente, 1, PERMISOS);

    if (semaforo == -1 || semaforo_cliente == -1) {
        fprintf(stderr, "ERROR: No se pudo conectar al servidor.\n");
        exit(1);
    }

    int shmid = shmget(llave, sizeof(struct Datos), PERMISOS);
    if (shmid == -1) {
        fprintf(stderr, "ERROR: El servidor no ha inicializado la memoria.\n");
        exit(1);
    }

    memoria = (struct Datos *) shmat(shmid, NULL, 0);
    if (memoria == (void *) -1) {
        fprintf(stderr, "Error al conectar memoria\n");
        exit(1);
    }

    /* Inicializar ncurses */
    initscr();
    start_color();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);

    inicializar_colores();

    /* Bucle principal - reemplaza la recursión */
    int continuar_programa = 1;
    
    while (continuar_programa) {
        int sesion_iniciada = 0;
        char usuario_actual[50] = {0};
        int mi_rol_admin = 0;

        /* BUCLE MENÚ INICIAL (Login / Registro) */
        while (!sesion_iniciada) {
            int opcion_ini = pantalla_menu_inicial();

            if (opcion_ini == 2 || opcion_ini == -1) {
                continuar_programa = 0;
                break;
            }

            int es_registro = (opcion_ini == 1);

            char usuario_local[50] = {0};
            char password_local[100] = {0};
            char correo_local[70] = {0};
            char password_cifrada[100] = {0};

            pantalla_login(usuario_local, password_local, correo_local, es_registro);

            if (usuario_local[0] == '\0' && password_local[0] == '\0' && correo_local[0] == '\0') {
                continue;
            }
            
            contrasenacif(password_local, password_cifrada);

            memoria->accion = es_registro ? REGISTRO : LOGIN;
            
            if (!es_registro) {
                if (correo_local[0] != '\0') {
                    strncpy(memoria->usuario, correo_local, 49);
                } else {
                    strncpy(memoria->usuario, usuario_local, 49);
                }
            } else {
                strncpy(memoria->usuario, usuario_local, 49);
            }
            
            strncpy(memoria->password, password_cifrada, 99);
            strncpy(memoria->correo, correo_local, 69);

            up(semaforo);
            down(semaforo_cliente);

            /* ═══ POPUP AÑADIDO PARA LOGIN ═══ */
            if (!es_registro) {
                if (strcmp(memoria->respuesta, "Login exitoso") == 0) {
                    sesion_iniciada = 1;
                    mi_rol_admin = memoria->es_admin;
                    strncpy(usuario_actual, memoria->usuario, 49);
                    popup_mensaje("ACCESO CONCEDIDO", "Bienvenid@ al sistema!", 0);
                } else {
                    popup_mensaje("ACCESO DENEGADO", memoria->respuesta, 1);
                }
            }
            /* ═══ POPUP AÑADIDO PARA REGISTRO ═══ */
            else {
                if (strstr(memoria->respuesta, "exitoso") != NULL ||
                    strstr(memoria->respuesta, "Exitoso") != NULL ||
                    strstr(memoria->respuesta, "OK") != NULL) {
                    popup_mensaje("REGISTRO EXITOSO", "Usuario creado correctamente. Inicia sesion.", 0);
                } else {
                    popup_mensaje("ERROR EN REGISTRO", memoria->respuesta, 1);
                }
            }
        }

        if (!continuar_programa) break;

        /* BUCLE MENÚ PRINCIPAL (usuario / admin) */
        while (sesion_iniciada) {
            int opcion = mi_rol_admin ? pantalla_menu_admin() : pantalla_menu_usuario(usuario_actual);

            if (opcion == -1) continue;

            int opcion_cerrar = mi_rol_admin ? 12 : 5;
            if (opcion == opcion_cerrar) {
                sesion_iniciada = 0;
                popup_mensaje("SESION CERRADA", "Hasta pronto!", 0);
                break;
            }

            /* ══════════════════════════════════════════════════════════
               OPCIONES USUARIO
               ══════════════════════════════════════════════════════════ */

            /* [0] Ver catálogo */
            if (!mi_rol_admin && opcion == 0) {
                memoria->accion = VER_PRODUCTOS;
                strncpy(memoria->usuario, usuario_actual, 49);
                up(semaforo);
                down(semaforo_cliente);

                if (strcmp(memoria->respuesta, "OK") == 0) {
                    struct Productos lista[20];
                    int total = 0;
                    char copia[1500];
                    strncpy(copia, memoria->productos, sizeof(copia) - 1);
                    char *linea = strtok(copia, "\n");
                    while (linea && total < 20) {
                        sscanf(linea, "%d;%[^;];%[^;];%f;%d",
                               &lista[total].id, lista[total].nombre,
                               lista[total].plataforma, &lista[total].precio,
                               &lista[total].stock);
                        total++;
                        linea = strtok(NULL, "\n");
                    }

                    int cantidad;
                    int idx = pantalla_catalogo(lista, total, &cantidad);
                    if (idx >= 0) {
                        memoria->accion = AGREGAR_CARRITO;
                        strncpy(memoria->usuario, usuario_actual, 49);
                        snprintf(memoria->productos, sizeof(memoria->productos),
                                 "%d;%d", lista[idx].id, cantidad);
                        up(semaforo);
                        down(semaforo_cliente);
                        popup_mensaje(
                            strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "CARRITO",
                            memoria->respuesta,
                            strstr(memoria->respuesta, "ERROR") != NULL);
                    }
                }
            }

            /* [1] Ver carrito */
            else if (!mi_rol_admin && opcion == 1) {
                memoria->accion = VER_CARRITO;
                strncpy(memoria->usuario, usuario_actual, 49);
                up(semaforo);
                down(semaforo_cliente);

                if (strcmp(memoria->respuesta, "OK") == 0) {
                    struct Productos carrito[50];
                    int total = 0;
                    float costo = 0.0f;
                    char copia[1500];
                    strncpy(copia, memoria->productos, sizeof(copia) - 1);
                    char *linea = strtok(copia, "\n");
                    while (linea && total < 50) {
                        sscanf(linea, "%d;%[^;];%[^;];%f;%d",
                               &carrito[total].id, carrito[total].nombre,
                               carrito[total].plataforma, &carrito[total].precio,
                               &carrito[total].stock);
                        costo += carrito[total].precio * carrito[total].stock;
                        total++;
                        linea = strtok(NULL, "\n");
                    }

                    int confirmar = pantalla_carrito(carrito, total, costo);
                    if (confirmar) {
                        memoria->accion = COMPRAR_CARRITO;
                        strncpy(memoria->usuario, usuario_actual, 49);
                        up(semaforo);
                        down(semaforo_cliente);
                        popup_mensaje("COMPRA", memoria->respuesta, 0);
                    }
                }
                else if (strcmp(memoria->respuesta, "VACIO") == 0) {
                    popup_mensaje("AVISO", "Tu carrito esta vacio", 1);
                }
                else {
                    popup_mensaje("ERROR", memoria->respuesta, 1);
                }
            }

            /* [2] Historial */
            else if (!mi_rol_admin && opcion == 2) {
                memoria->accion = VER_HISTORIAL;
                strncpy(memoria->usuario, usuario_actual, 49);
                up(semaforo);
                down(semaforo_cliente);

                if (strcmp(memoria->respuesta, "OK") == 0)
                    pantalla_texto("HISTORIAL DE COMPRAS", memoria->productos);
                else if (strcmp(memoria->respuesta, "VACIO") == 0)
                    popup_mensaje("AVISO", "Aun no tienes compras.", 1);
                else
                    popup_mensaje("ERROR", memoria->respuesta, 1);
            }

            /* [3] Buscar producto (usuario) */
            else if (!mi_rol_admin && opcion == 3) {
                const char *campos[] = {"Nombre del producto"};
                char buf[200] = {0};
                char *vals[] = {buf};
                const int maxs[] = {199};
                const int pass[] = {0};

                if (pantalla_formulario("BUSCAR PRODUCTO", campos, vals, maxs, pass, 1)) {
                    strncpy(memoria->productos, buf, 199);
                    memoria->accion = BUSCAR_PRODUCTO;
                    up(semaforo);
                    down(semaforo_cliente);
                    pantalla_texto("RESULTADOS DE BUSQUEDA", memoria->productos);
                } else {
                    popup_mensaje("AVISO", "Busqueda cancelada.", 1);
                }
                clear();
                refresh();
            }

            /* [4] Modificar mi cuenta */
            else if (!mi_rol_admin && opcion == 4) {
                const char *campos[] = {"Nuevo usuario", "Nueva password", "Nuevo correo"};
                char nu[50] = {0}, np[100] = {0}, nc[70] = {0};
                char *vals[] = {nu, np, nc};
                const int maxs[] = {49, 99, 69};
                const int pass[] = {0, 1, 0};

                while (1) {
                    if (!pantalla_formulario("MODIFICAR MI CUENTA", campos, vals, maxs, pass, 3)) {
                        popup_mensaje("AVISO", "Operacion cancelada.", 1);
                        break;
                    }

                    if (strchr(nc, '@') == NULL) {
                        popup_mensaje("ERROR", "El correo debe contener '@'.", 1);
                        continue;
                    }

                    char np_cif[100] = {0};
                    contrasenacif(np, np_cif);

                    strncpy(memoria->usuario, nu, 49);
                    strncpy(memoria->password, np_cif, 99);
                    strncpy(memoria->correo, nc, 69);
                    strncpy(memoria->mensaje, usuario_actual, 49);
                    memoria->accion = MODIFICAR_MI_CUENTA;

                    up(semaforo);
                    down(semaforo_cliente);

                    if (strstr(memoria->respuesta, "ERROR") != NULL) {
                        popup_mensaje("ERROR", memoria->respuesta, 1);
                    } else {
                        strncpy(usuario_actual, memoria->usuario, 49);
                        popup_mensaje("EXITO", memoria->respuesta, 0);
                    }
                    break;
                }
            }

            /* ══════════════════════════════════════════════════════════
               OPCIONES ADMINISTRADOR
               ══════════════════════════════════════════════════════════ */

            /* [0] Ver catálogo admin */
            else if (mi_rol_admin && opcion == 0) {
                memoria->accion = VER_PRODUCTOS;
                strncpy(memoria->usuario, usuario_actual, 49);
                up(semaforo);
                down(semaforo_cliente);

                if (strcmp(memoria->respuesta, "OK") == 0) {
                    struct Productos lista[20];
                    int total = 0;
                    char copia[1500];
                    strncpy(copia, memoria->productos, sizeof(copia) - 1);
                    char *linea = strtok(copia, "\n");
                    while (linea && total < 20) {
                        sscanf(linea, "%d;%[^;];%[^;];%f;%d",
                               &lista[total].id, lista[total].nombre,
                               lista[total].plataforma, &lista[total].precio,
                               &lista[total].stock);
                        total++;
                        linea = strtok(NULL, "\n");
                    }
                    int dummy;
                    pantalla_catalogo(lista, total, &dummy);
                }
            }

            /* [1] Agregar producto */
            else if (mi_rol_admin && opcion == 1) {
                const char *campos[] = {"ID", "Nombre", "Plataforma", "Precio", "Stock"};
                char sid[10] = {0}, nom[50] = {0}, pla[50] = {0}, pre[20] = {0}, sto[10] = {0};
                char *vals[] = {sid, nom, pla, pre, sto};
                const int maxs[] = {9, 49, 49, 19, 9};
                const int pass[] = {0, 0, 0, 0, 0};

                if (pantalla_formulario("AGREGAR PRODUCTO", campos, vals, maxs, pass, 5)) {
                    memoria->producto.id = atoi(sid);
                    strncpy(memoria->producto.nombre, nom, 49);
                    strncpy(memoria->producto.plataforma, pla, 49);
                    memoria->producto.precio = (float)atof(pre);
                    memoria->producto.stock = atoi(sto);

                    memoria->accion = AGREGAR_PRODUCTO_ADMIN;
                    up(semaforo);
                    down(semaforo_cliente);

                    popup_mensaje(
                        strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "PRODUCTO",
                        memoria->respuesta,
                        strstr(memoria->respuesta, "ERROR") != NULL);
                } else {
                    popup_mensaje("AVISO", "Operacion cancelada.", 1);
                }
            }

            /* [2] Modificar stock */
            else if (mi_rol_admin && opcion == 2) {
                const char *campos[] = {"ID del producto", "Nuevo stock"};
                char sid[10] = {0}, sst[10] = {0};
                char *vals[] = {sid, sst};
                const int maxs[] = {9, 9};
                const int pass[] = {0, 0};

                if (pantalla_formulario("MODIFICAR STOCK", campos, vals, maxs, pass, 2)) {
                    memoria->producto.id = atoi(sid);
                    memoria->producto.stock = atoi(sst);
                    memoria->accion = MODIFICAR_STOCK;

                    up(semaforo);
                    down(semaforo_cliente);

                    popup_mensaje(
                        strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "STOCK",
                        memoria->respuesta,
                        strstr(memoria->respuesta, "ERROR") != NULL);
                } else {
                    popup_mensaje("AVISO", "Modificacion cancelada.", 1);
                }
                clear();
                refresh();
            }

            /* [3] Eliminar producto */
            else if (mi_rol_admin && opcion == 3) {
                const char *campos[] = {"ID del producto a eliminar"};
                char sid[10] = {0};
                char *vals[] = {sid};
                const int maxs[] = {9};
                const int pass[] = {0};

                if (pantalla_formulario("ELIMINAR PRODUCTO", campos, vals, maxs, pass, 1)) {
                    memoria->producto.id = atoi(sid);
                    memoria->accion = ELIMINAR_PRODUCTO;

                    up(semaforo);
                    down(semaforo_cliente);

                    popup_mensaje(
                        strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "PRODUCTO",
                        memoria->respuesta,
                        strstr(memoria->respuesta, "ERROR") != NULL);
                } else {
                    popup_mensaje("AVISO", "Eliminacion cancelada.", 1);
                }
                clear();
                refresh();
            }

            /* [4] Buscar producto (admin) */
            else if (mi_rol_admin && opcion == 4) {
                const char *campos[] = {"Nombre del producto"};
                char buf[200] = {0};
                char *vals[] = {buf};
                const int maxs[] = {199};
                const int pass[] = {0};

                if (pantalla_formulario("BUSCAR PRODUCTO", campos, vals, maxs, pass, 1)) {
                    strncpy(memoria->productos, buf, 199);
                    memoria->accion = BUSCAR_PRODUCTO;

                    up(semaforo);
                    down(semaforo_cliente);

                    pantalla_texto("RESULTADOS", memoria->productos);
                } else {
                    popup_mensaje("AVISO", "Busqueda cancelada.", 1);
                }
                clear();
                refresh();
            }

            /* [5] Ver usuarios */
            else if (mi_rol_admin && opcion == 5) {
                memoria->accion = VER_USUARIOS;
                up(semaforo);
                down(semaforo_cliente);
                pantalla_texto("USUARIOS REGISTRADOS", memoria->productos);
            }

            /* [6] Crear usuario */
            else if (mi_rol_admin && opcion == 6) {
                const char *campos[] = {"Usuario", "Password", "Correo"};
                char nu[50] = {0}, np[100] = {0}, nc[70] = {0};
                char *vals[] = {nu, np, nc};
                const int maxs[] = {49, 99, 69};
                const int pass[] = {0, 1, 0};

                if (pantalla_formulario("CREAR USUARIO", campos, vals, maxs, pass, 3)) {
                    char np_cif[100] = {0};
                    contrasenacif(np, np_cif);

                    strncpy(memoria->usuario, nu, 49);
                    strncpy(memoria->password, np_cif, 99);
                    strncpy(memoria->correo, nc, 69);

                    memoria->accion = CREAR_USUARIO_ADMIN;
                    up(semaforo);
                    down(semaforo_cliente);

                    popup_mensaje(
                        strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "USUARIO",
                        memoria->respuesta,
                        strstr(memoria->respuesta, "ERROR") != NULL);
                } else {
                    popup_mensaje("AVISO", "Creacion cancelada.", 1);
                }
                clear();
                refresh();
            }

            /* [7] Modificar usuario */
            else if (mi_rol_admin && opcion == 7) {
                const char *campos[] = {"Usuario a modificar", "Nuevo usuario", "Nueva password", "Nuevo correo"};
                char vm[50] = {0}, nu[50] = {0}, np[100] = {0}, nc[70] = {0};
                char *vals[] = {vm, nu, np, nc};
                const int maxs[] = {49, 49, 99, 69};
                const int pass[] = {0, 0, 1, 0};

                while (1) {
                    if (pantalla_formulario("MODIFICAR USUARIO (ADMIN)", campos, vals, maxs, pass, 4)) {
                        if (strchr(nc, '@') == NULL) {
                            popup_mensaje("ERROR", "El correo debe contener '@'.", 1);
                            continue;
                        }

                        char np_cif[100] = {0};
                        contrasenacif(np, np_cif);

                        strncpy(memoria->mensaje, vm, 49);
                        strncpy(memoria->usuario, nu, 49);
                        strncpy(memoria->password, np_cif, 99);
                        strncpy(memoria->correo, nc, 69);

                        memoria->accion = MODIFICAR_USUARIO;
                        up(semaforo);
                        down(semaforo_cliente);

                        popup_mensaje(
                            strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "USUARIO",
                            memoria->respuesta,
                            strstr(memoria->respuesta, "ERROR") != NULL);
                        break;
                    } else {
                        popup_mensaje("AVISO", "Modificacion cancelada.", 1);
                        break;
                    }
                }
                clear();
                refresh();
            }

            /* [8] Eliminar usuario */
            else if (mi_rol_admin && opcion == 8) {
                const char *campos[] = {"Usuario a eliminar"};
                char buf[50] = {0};
                char *vals[] = {buf};
                const int maxs[] = {49};
                const int pass[] = {0};

                if (pantalla_formulario("ELIMINAR USUARIO", campos, vals, maxs, pass, 1)) {
                    strncpy(memoria->mensaje, buf, 49);
                    memoria->accion = ELIMINAR_USUARIO;

                    up(semaforo);
                    down(semaforo_cliente);

                    popup_mensaje(
                        strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "USUARIO",
                        memoria->respuesta,
                        strstr(memoria->respuesta, "ERROR") != NULL);
                } else {
                    popup_mensaje("AVISO", "Eliminacion cancelada.", 1);
                }
                clear();
                refresh();
            }

            /* [9] Reporte diario */
            else if (mi_rol_admin && opcion == 9) {
                memoria->accion = REPORTE_DIARIO;
                up(semaforo);
                down(semaforo_cliente);
                pantalla_texto("REPORTE VENTAS DIARIAS", memoria->productos);
            }

            /* [10] Reporte semanal */
            else if (mi_rol_admin && opcion == 10) {
                memoria->accion = REPORTE_SEMANAL;
                up(semaforo);
                down(semaforo_cliente);
                pantalla_texto("REPORTE VENTAS SEMANALES", memoria->productos);
            }

            /* [11] Reporte mensual */
            else if (mi_rol_admin && opcion == 11) {
                memoria->accion = REPORTE_MENSUAL;
                up(semaforo);
                down(semaforo_cliente);
                pantalla_texto("REPORTE VENTAS MENSUALES", memoria->productos);
            }

        } /* fin while sesion_iniciada */

    } /* fin while continuar_programa */

    endwin();
    shmdt(memoria);
    return 0;
}
