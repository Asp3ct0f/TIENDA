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
    char productos[8000];
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
void contrasenacif(char *password, char *cifrada);

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

    int pos = 0;
    int ch;

    // Inicializamos el buffer vacío por seguridad
    memset(buf, 0, max);

    while (1) {
        wmove(w, fila, col + pos);
        ch = wgetch(w); // Lee carácter por carácter en tiempo real

        // 1. SI PRESIONA ESC (Código 27) -> SALIR INMEDIATAMENTE
        if (ch == 27) {
            noecho();
            curs_set(0);
            wattroff(w, COLOR_PAIR(COLOR_NORMAL));
            return 0; // Regresa 0 para detonar la cancelación en el formulario
        }

        // 2. SI PRESIONA ENTER -> TERMINAR DE LEER
        if (ch == '\n' || ch == KEY_ENTER) {
            break;
        }

        // 3. SI PRESIONA BORRAR (Backspace)
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                mvwprintw(w, fila, col + pos, " "); // Borra visualmente en la ventana
            }
        } 
        // 4. SI ES UN CARÁCTER IMPRIMIBLE Y NO EXCEDE EL MÁXIMO
        else if (isprint(ch) && pos < (max - 1)) {
            buf[pos] = ch;
            mvwprintw(w, fila, col + pos, "%c", ch);
            pos++;
        }
    }

    wattroff(w, COLOR_PAIR(COLOR_NORMAL));
    noecho();
    curs_set(0);

    // Limpieza de saltos de línea por si acaso
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

    // Con 'static', la función RECORDARÁ este valor entre llamadas consecutivas.
    // -1 = No ha elegido (Submenú Raíz), 0 = Sección Productos, 1 = Sección Usuarios
    static int seccion_guardada = -1; 

    while (1) {
        // 1. SI NO HAY SECCIÓN GUARDADA, PINTAMOS EL SUBMENÚ RAÍZ
        if (seccion_guardada == -1) {
            int alto_sub = 10, ancho_sub = 44;
            WINDOW *w_sub = newwin(alto_sub, ancho_sub, (filas - alto_sub) / 2, (cols - ancho_sub) / 2);
            keypad(w_sub, TRUE);

            const char *opciones_raiz[] = {
                "1. Gestionar Productos",
                "2. Gestionar Usuarios y Reportes",
                "3. Cerrar Sesion"
            };

            int seleccion_raiz = menu_navegar(w_sub, opciones_raiz, 3, "SUBMENU ADMIN");
            delwin(w_sub);
            clear(); refresh();

            // Si presiona ESC en el submenú raíz o elige Cerrar Sesión, salimos al main con 12
            if (seleccion_raiz == 2 || seleccion_raiz == -1) {
                return 12; 
            }

            // Guardamos la elección en nuestra variable estática
            seccion_guardada = seleccion_raiz;
        }

        // 2. RAMA DE PRODUCTOS (seccion_guardada == 0)
        if (seccion_guardada == 0) {
            int alto_p = 14, ancho_p = 44;
            WINDOW *w_p = newwin(alto_p, ancho_p, (filas - alto_p) / 2, (cols - ancho_p) / 2);
            keypad(w_p, TRUE);

            const char *opciones_prod[] = {
                "Ver catalogo",      // Mapea a 0
                "Agregar producto",  // Mapea a 1
                "Modificar stock",   // Mapea a 2
                "Eliminar producto", // Mapea a 3
                "Buscar producto",   // Mapea a 4
                "<-- Volver atras"
            };

            int r = menu_navegar(w_p, opciones_prod, 6, "PRODUCTOS");
            delwin(w_p);
            clear(); refresh();

            // Si presiona ESC en el menú de productos o elige "Volver atrás"
            if (r == -1 || r == 5) {
                seccion_guardada = -1; // Reseteamos para que pinte el submenú raíz
                continue;              // Regresa al inicio del while(1) interno
            }

            return r; // Devuelve la opción al main (0, 1, 2, 3 o 4)
        }

        // 3. RAMA DE USUARIOS Y REPORTES (seccion_guardada == 1)
        if (seccion_guardada == 1) {
            int alto_u = 18, ancho_u = 44;
            WINDOW *w_u = newwin(alto_u, ancho_u, (filas - alto_u) / 2, (cols - ancho_u) / 2);
            keypad(w_u, TRUE);

            const char *opciones_user[] = {
                "Ver usuarios",             // Mapea a 5
                "Crear usuario",            // Mapea a 6
                "Modificar usuario",        // Mapea a 7
                "Eliminar usuario",         // Mapea a 8
                "Reporte ventas diarias",   // Mapea a 9
                "Reporte ventas semanales", // Mapea a 10
                "Reporte ventas mensuales",// Mapea a 11
                "<-- Volver atras"
            };

            int r = menu_navegar(w_u, opciones_user, 8, "USUARIOS Y REPORTES");
            delwin(w_u);
            clear(); refresh();

            // Si presiona ESC en el menú de usuarios o elige "Volver atrás"
            if (r == -1 || r == 7) {
                seccion_guardada = -1; // Reseteamos para que pinte el submenú raíz
                continue;              // Regresa al inicio del while(1) interno
            }

            return r + 5; // Mapea y devuelve el índice mapeado al main (5 al 11)
        }
    }

    return 12;
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
    
    wclear(w);
    dibujar_ventana(w, titulo);

    // Reservamos espacio e inicializamos el búfer con ceros
    char copia[8000];
    memset(copia, 0, sizeof(copia)); 
    
    if (texto != NULL) {
        strncpy(copia, texto, sizeof(copia) - 1);
    }

    char *linea = strtok(copia, "\n");
    int fila_actual = 2;

    while (linea != NULL) {
        int linea_valida = 1;
        
        // Validar caracteres (permitiendo tabulador ASCII 9)
        for (size_t i = 0; i < strlen(linea); i++) {
            unsigned char c = (unsigned char)linea[i];
            if (c < 32 || c > 126) {
                if (c != 9) { 
                    linea_valida = 0;
                    break;
                }
            }
        }

        if (linea_valida && strlen(linea) > 0) {
            // Si la siguiente línea va a desbordar el marco inferior
            if (fila_actual >= alto - 3) {
                wattron(w, COLOR_PAIR(COLOR_NORMAL) | A_BOLD);
                mvwprintw(w, alto - 2, 2, " -- Presione una tecla para ver mas... -- ");
                wattroff(w, COLOR_PAIR(COLOR_NORMAL) | A_BOLD);
                
                wrefresh(w);
                wgetch(w); // Espera interactiva para cambiar de página
                
                wclear(w);
                dibujar_ventana(w, titulo);
                fila_actual = 2; // Reiniciamos arriba en la nueva página
            }

            wattron(w, COLOR_PAIR(COLOR_NORMAL));
            // Forzamos la sustitución de tabuladores por espacios para que ncurses no rompa el diseño
            int col_offset = 2;
            for (size_t j = 0; j < strlen(linea) && col_offset < (ancho - 3); j++) {
                if (linea[j] == '\t') {
                    mvwprintw(w, fila_actual, col_offset, "    ");
                    col_offset += 4;
                } else {
                    mvwaddch(w, fila_actual, col_offset, linea[j]);
                    col_offset++;
                }
            }
            wattroff(w, COLOR_PAIR(COLOR_NORMAL));
            fila_actual++;
        }
        
        linea = strtok(NULL, "\n");
    }

    // ─── AQUÍ SE ASEGURA LA VISUALIZACIÓN DEL TOTAL ───
    // Imprimimos la leyenda de salida exactamente abajo en la página donde quedó el total
    wattron(w, COLOR_PAIR(COLOR_NORMAL) | A_REVERSE | A_BOLD);
    mvwprintw(w, alto - 2, 2, " Fin del reporte. Presione cualquier tecla para regresar... ");
    wattroff(w, COLOR_PAIR(COLOR_NORMAL) | A_REVERSE | A_BOLD);
    
    // Forzamos el refresco completo antes de la última pausa
    wrefresh(w);
    wgetch(w);
    
    // Limpieza al salir de la pantalla
    wclear(w);
    wrefresh(w);
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
        mvwprintw(w, 2 + i * 3, 2 + (int)strlen(campos[i]) + 4, "[ESC para salir]");
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

int pantalla_formulario_win(WINDOW *w, const char *titulo, const char **campos, char **valores,
                            const int *maxlen, const int *es_pass, int n_campos)
{
    keypad(w, TRUE);
    dibujar_ventana(w, titulo);

    int exito = 1;
    for (int i = 0; i < n_campos; i++) {
        wattron(w, COLOR_PAIR(COLOR_TITULO));
        mvwprintw(w, 2 + i * 3, 2, "> %s:", campos[i]);
        wattroff(w, COLOR_PAIR(COLOR_TITULO));
        
        wattron(w, A_DIM);
        mvwprintw(w, 2 + i * 3, 2 + (int)strlen(campos[i]) + 4, "[ESC]");
        wattroff(w, A_DIM);

        if (es_pass[i])
            exito = leer_password(w, 3 + i * 3, 2, valores[i], maxlen[i]);
        else
            exito = leer_cadena(w, 3 + i * 3, 2, valores[i], maxlen[i]);
        
        if (!exito) break;
    }
    return exito;
}


int pantalla_formulario_con_catalogo(const char *titulo, const char **campos, char **valores,
                                     const int *maxlen, const int *es_pass, int n_campos,
                                     int semaforo, int semaforo_cliente, struct Datos *memoria)
{
    // 1. SOLICITAR CATÁLOGO ACTUALIZADO AL SERVIDOR ANTES DE DIBUJAR
    memoria->accion = VER_PRODUCTOS;
    up(semaforo);
    int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    return 0;
}

    int filas, cols;
    getmaxyx(stdscr, filas, cols);
    clear();

    // 2. DIVIDIR LA PANTALLA EN DOS MITADES PERFECTAS
    int ancho_mitad = cols / 2;
    WINDOW *w_cat = newwin(filas - 2, ancho_mitad - 1, 1, 1);
    WINDOW *w_form = newwin(filas - 2, ancho_mitad - 1, 1, ancho_mitad + 1);

    // 3. PINTAR EL CATÁLOGO EN LA VENTANA IZQUIERDA
    dibujar_ventana(w_cat, "CATALOGO EN TIEMPO REAL");
    
    char copia_cat[8000] = {0}; // Usamos el buffer amplio que corregimos antes
    strncpy(copia_cat, memoria->productos, sizeof(copia_cat) - 1);
    
    char *linea_c = strtok(copia_cat, "\n");
    int f_c = 2;
    
    wattron(w_cat, COLOR_PAIR(COLOR_NORMAL));
    while (linea_c != NULL && f_c < (filas - 4)) {
        mvwprintw(w_cat, f_c++, 2, "%-.*s", ancho_mitad - 5, linea_c);
        linea_c = strtok(NULL, "\n");
    }
    wattroff(w_cat, COLOR_PAIR(COLOR_NORMAL));
    wrefresh(w_cat); // Lo dejamos estático en pantalla

    // 4. LANZAR EL FORMULARIO ADAPTADO EN LA VENTANA DERECHA
    int exito = pantalla_formulario_win(w_form, titulo, campos, valores, maxlen, es_pass, n_campos);

    // 5. LIMPIEZA DE VENTANAS
    delwin(w_cat);
    delwin(w_form);
    clear();
    refresh();

    return exito; // Retorna 1 si se llenó con éxito, 0 si presionó ESC
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
            int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

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
                int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

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
                        int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}
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
                int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

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
                        int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}
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
                int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

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

                // Si se llena con éxito entra al IF, si presionas ESC entra al ELSE
                if (pantalla_formulario("BUSCAR PRODUCTO", campos, vals, maxs, pass, 1)) {
                    strncpy(memoria->productos, buf, 199);
                    memoria->accion = BUSCAR_PRODUCTO;
                    up(semaforo);
                    int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}
                    pantalla_texto("RESULTADOS DE BUSQUEDA", memoria->productos);
                } else {
                    // Muestra el popup de aviso
                    popup_mensaje("AVISO", "Busqueda cancelada.", 1);
                    // Al darle ENTER a este popup, continuará hacia abajo y saldrá al menú
                }
                
                // Limpia los restos de la ventana para pintar el menú principal limpio
                clear();
                refresh();
            }

            /* [4] Modificar mi cuenta */
            else if (!mi_rol_admin && opcion == 4) {

                char nu[50] = {0};
                char np[100] = {0};
                char nc[70] = {0};

                
                
                pantalla_login(nu,np,nc,1);


                if (nu[0] == '\0' ||np[0] == '\0' ||nc[0] == '\0') {

                    popup_mensaje("AVISO","Operacion cancelada.",1
                    );

                    continue;
                }


                if (strchr(nc, '@') == NULL) {
                    popup_mensaje(
                        "ERROR",
                        "El correo debe contener '@'.",
                        1
                    );
                    continue;
                }


                char np_cif[100] = {0};
                contrasenacif(np, np_cif);


                strncpy(memoria->usuario, nu, 49);
                strncpy(memoria->password, np_cif, 99);
                strncpy(memoria->correo, nc, 69);

                /* Usuario actual que se quiere modificar */
                strncpy(memoria->mensaje, usuario_actual, 49);

                memoria->accion = MODIFICAR_MI_CUENTA;


                up(semaforo);
                int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}


                if (strstr(memoria->respuesta, "ERROR") != NULL) {

                    popup_mensaje(
                        "ERROR",
                        memoria->respuesta,
                        1
                    );

                } else {

                    strncpy(usuario_actual, memoria->usuario, 49);

                    popup_mensaje(
                        "EXITO",
                        memoria->respuesta,
                        0
                    );
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
                int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

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
                // Pedimos los 5 datos necesarios (incluyendo el ID si tu sistema lo requiere manual)
                const char *campos[] = {"ID Producto (Numero)", "Nombre", "Plataforma", "Precio", "Stock"};
                char buf_id[20] = {0}, bn[50] = {0}, bp[50] = {0}, bpr[20] = {0}, bs[20] = {0};
                char *valores[] = {buf_id, bn, bp, bpr, bs};
                const int maxlen[] = {19, 49, 49, 19, 19};
                const int es_pass[] = {0, 0, 0, 0, 0};

                // Llamamos a la función con el catálogo al lado
                if (pantalla_formulario_con_catalogo("AGREGAR PRODUCTO", campos, valores, maxlen, es_pass, 5, semaforo, semaforo_cliente, memoria)) {
                    
                    // ─── AQUÍ ESTÁ EL TRUCO: Mapear correctamente a la estructura 'producto' ───
                    memoria->producto.id = atoi(buf_id); 
                    strncpy(memoria->producto.nombre, bn, sizeof(memoria->producto.nombre) - 1);
                    strncpy(memoria->producto.plataforma, bp, sizeof(memoria->producto.plataforma) - 1);
                    memoria->producto.precio = atof(bpr);
                    memoria->producto.stock = atoi(bs);

                    memoria->accion = AGREGAR_PRODUCTO_ADMIN;
                    up(semaforo);
                    int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

                    popup_mensaje(strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "PRODUCTO", 
                                  memoria->respuesta, 
                                  strstr(memoria->respuesta, "ERROR") != NULL);
                } else {
                    popup_mensaje("AVISO", "Operacion cancelada.", 1);
                }
                clear(); refresh();
            }
            /* [2] Modificar stock (Mapeo correcto a estructura producto) */
            else if (mi_rol_admin && opcion == 2) {
                const char *campos[] = {"ID Producto", "Nuevo Stock"};
                char buf_id[20] = {0}, buf_st[20] = {0};
                char *valores[] = {buf_id, buf_st};
                const int maxlen[] = {19, 19};
                const int es_pass[] = {0, 0};

                // Llamamos a la función con catálogo al lado
                if (pantalla_formulario_con_catalogo("MODIFICAR STOCK", campos, valores, maxlen, es_pass, 2, semaforo, semaforo_cliente, memoria)) {
                    
                    // ─── MAPEO CORRECTO HACIA LA ESTRUCTURA DE PRODUCTO ───
                    memoria->producto.id = atoi(buf_id);     // Convertimos ID a entero
                    memoria->producto.stock = atoi(buf_st);   // Convertimos Stock a entero
                    
                    // Mantenemos estos por si acaso tu servidor viejo los usaba de respaldo
                    strncpy(memoria->mensaje, buf_id, 49);
                    strncpy(memoria->password, buf_st, 99);
                    
                    memoria->accion = MODIFICAR_STOCK;
                    up(semaforo);
                    int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

                    popup_mensaje(strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "STOCK", 
                                  memoria->respuesta, 
                                  strstr(memoria->respuesta, "ERROR") != NULL);
                } else {
                    popup_mensaje("AVISO", "Operacion cancelada.", 1);
                }
                clear(); refresh();
            }
        /* [3] Eliminar producto  */
            else if (mi_rol_admin && opcion == 3) {
                const char *campos[] = {"ID de Producto a Eliminar"};
                char buf_id[20] = {0};
                char *valores[] = {buf_id};
                const int maxlen[] = {19};
                const int es_pass[] = {0};

                // Llamamos a la función con catálogo al lado
                if (pantalla_formulario_con_catalogo("ELIMINAR PRODUCTO", campos, valores, maxlen, es_pass, 1, semaforo, semaforo_cliente, memoria)) {
                    
                    // ─── MAPEO CORRECTO HACIA LA ESTRUCTURA DE PRODUCTO ───
                    memoria->producto.id = atoi(buf_id); // Convertimos el ID capturado a entero
                    
                    // Mantenemos este por si acaso tu servidor de respaldo aún leía de ahí
                    strncpy(memoria->mensaje, buf_id, 49);
                    
                    memoria->accion = ELIMINAR_PRODUCTO;
                    up(semaforo);
                    int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

                    popup_mensaje(strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "ELIMINAR", 
                                  memoria->respuesta, 
                                  strstr(memoria->respuesta, "ERROR") != NULL);
                } else {
                    popup_mensaje("AVISO", "Eliminacion cancelada.", 1);
                }
                clear(); refresh();
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
                    int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

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
                int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}
                pantalla_texto("USUARIOS REGISTRADOS", memoria->productos);
            }

            /* [6] Crear usuario */
            else if (mi_rol_admin && opcion == 6) {
                char nu[50] = {0}; // Buffer para el nuevo usuario
                char np[100] = {0};// Buffer para la contraseña
                char nc[70] = {0}; // Buffer para el correo

                // Desplegamos tu pantalla_login intacta en modo registro (1)
                // Esto aplica tus filtros interactivos automáticos para el Administrador
                pantalla_login(nu, np, nc, 1);

                // Si el administrador canceló presionando ESC dentro de la pantalla
                if (nu[0] == '\0' || np[0] == '\0' || nc[0] == '\0') {
                    popup_mensaje("AVISO", "Creacion cancelada.", 1);
                } else {
                    // Ciframos la contraseña segura obtenida de la pantalla interactiva
                    char np_cif[100] = {0};
                    contrasenacif(np, np_cif);

                    // Copiamos los datos validados a la memoria compartida
                    strncpy(memoria->usuario, nu, 49);
                    strncpy(memoria->password, np_cif, 99);
                    strncpy(memoria->correo, nc, 69);

                    memoria->accion = CREAR_USUARIO_ADMIN;
                    
                    // Sincronización IPC con el servidor
                    up(semaforo);
                    int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

                    // Desplegar la respuesta que devuelva el Servidor
                    popup_mensaje(
                        strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "USUARIO",
                        memoria->respuesta,
                        strstr(memoria->respuesta, "ERROR") != NULL);
                }
                clear();
                refresh();
            }

            /* [7] Modificar usuario */
            else if (mi_rol_admin && opcion == 7) {
                char nu[50] = {0}; // Nuevo usuario
                char np[100] = {0};// Nueva contraseña
                char nc[70] = {0}; // Nuevo correo

                // Desplegamos tu pantalla_login intacta en modo registro (1)
                // Esto te da todas tus validaciones en tiempo real (.com y contraseña fuerte)
                pantalla_login(nu, np, nc, 1);

                // Si el administrador canceló presionando ESC dentro de la pantalla
                if (nu[0] == '\0' || np[0] == '\0' || nc[0] == '\0') {
                    popup_mensaje("AVISO", "Modificacion cancelada.", 1);
                } else {
                    // Ciframos la contraseña que ya fue validada por tus filtros interactivos
                    char np_cif[100] = {0};
                    contrasenacif(np, np_cif);

                    // Al usar pantalla_login directamente, asumimos que el usuario a buscar 
                    // es el mismo nombre nuevo que se está ingresando o el que se desea actualizar
                    strncpy(memoria->mensaje, nu, 49);     // Usuario a buscar en la base de datos
                    strncpy(memoria->usuario, nu, 49);     // Nuevo nombre de usuario
                    strncpy(memoria->password, np_cif, 99); // Nueva contraseña cifrada
                    strncpy(memoria->correo, nc, 69);       // Nuevo correo verificado

                    memoria->accion = MODIFICAR_USUARIO;
                    
                    // Sincronización IPC con el servidor
                    up(semaforo);
                    int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}


                    // Desplegar la respuesta que envíe el Servidor
                    popup_mensaje(
                        strstr(memoria->respuesta, "ERROR") != NULL ? "ERROR" : "USUARIO",
                        memoria->respuesta,
                        strstr(memoria->respuesta, "ERROR") != NULL);
                }
                clear();
                refresh();
            }

            /* [8] Eliminar usuario */
            else if (mi_rol_admin && opcion == 8) {
                // Cambiamos la etiqueta del formulario para que pida el Correo
                const char *campos[] = {"Correo del usuario a eliminar"};
                char buf[70] = {0}; // Usamos 70 caracteres que es el tamaño de memoria->correo
                char *vals[] = {buf};
                const int maxs[] = {69};
                const int pass[] = {0};

                if (pantalla_formulario("ELIMINAR USUARIO", campos, vals, maxs, pass, 1)) {
                    
                    // Validación local rápida: verificar que al menos tenga un '@'
                    if (strchr(buf, '@') == NULL) {
                        popup_mensaje("ERROR", "El correo debe contener '@'.", 1);
                        clear();
                        refresh();
                        continue; // Evita enviar datos inválidos y te regresa al menú
                    }

                    // Copiamos el dato a memoria->correo para el nuevo código del servidor
                    strncpy(memoria->correo, buf, 69);
                    
                    // Mantenemos este por si acaso alguna otra función dependía de él
                    strncpy(memoria->mensaje, buf, 49); 

                    memoria->accion = ELIMINAR_USUARIO;

                    // Sincronización mediante semáforos
                    up(semaforo);
                    int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}

                    // Mostramos la respuesta devuelta por el servidor
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
                int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}
                pantalla_texto("REPORTE VENTAS DIARIAS", memoria->productos);
            }

            /* [10] Reporte semanal */
            else if (mi_rol_admin && opcion == 10) {
                memoria->accion = REPORTE_SEMANAL;
                up(semaforo);
                int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}
                pantalla_texto("REPORTE VENTAS SEMANALES", memoria->productos);
            }

            /* [11] Reporte mensual */
            else if (mi_rol_admin && opcion == 11) {
                memoria->accion = REPORTE_MENSUAL;
                up(semaforo);
                int respuesta_recibida = 0;

for(int i = 0; i < 5; i++)
{
    sleep(1);

    if(semctl(semaforo_cliente, 0, GETVAL) > 0)
    {
        down(semaforo_cliente);

        respuesta_recibida = 1;

        break;
    }
}

if(!respuesta_recibida)
{
    clear();

    mvprintw(LINES/2 - 1, (COLS - 35)/2,
             "ERROR: Servidor no disponible");

    mvprintw(LINES/2 + 1, (COLS - 30)/2,
             "Presione una tecla para continuar");

    refresh();

    getch();

    continue;
}
                pantalla_texto("REPORTE VENTAS MENSUALES", memoria->productos);
            }

        } /* fin while sesion_iniciada */

    } /* fin while continuar_programa */

    endwin();
    shmdt(memoria);
    return 0;
}
