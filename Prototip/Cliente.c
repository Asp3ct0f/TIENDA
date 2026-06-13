#include <stdio.h>


#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>
#include <string.h>
#include <ncurses.h>
#include "cifrado.h"
#include <ctype.h>
#include<strings.h>
#include <ctype.h>
#define PERMISOS 0644
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

/* ── Colores ──────────────────────────────────────────────── */
#ifndef CONFIG_UI_H
#define CONFIG_UI_H

#define COLOR_TITULO   1
#define COLOR_NORMAL   2
#define COLOR_SELEC    3
#define COLOR_ERROR    4
#define COLOR_OK       5
#define COLOR_BORDE    6

// Función para inicializar los colores una sola vez
void inicializar_colores() {
    start_color();
    init_pair(COLOR_TITULO, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_NORMAL, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_SELEC,  COLOR_BLACK, COLOR_GREEN);
    init_pair(COLOR_ERROR,  COLOR_RED,   COLOR_BLACK);
    init_pair(COLOR_OK,     COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_BORDE,  COLOR_GREEN, COLOR_BLACK);
}

#endif


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

/* ── Semáforos ────────────────────────────────────────────── */
void up(int semid) {
    struct sembuf op = {0, +1, 0};
    semop(semid, &op, 1);
}
void down(int semid) {
    struct sembuf op = {0, -1, 0};
    semop(semid, &op, 1);
}

/*
   UTILIDADES ncurses (Estilo Matrix)
  */

/* Dibuja un recuadro con título centrado */
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

void popup_mensaje(const char *titulo,
                   const char *mensaje,
                   int es_error)
{
    int filas, cols;

    getmaxyx(stdscr, filas, cols);

    int alto  = 9;
    int ancho = 60;

    int y = (filas - alto) / 2;
    int x = (cols  - ancho) / 2;

    WINDOW *popup = newwin(alto, ancho, y, x);

    keypad(popup, TRUE);

    dibujar_ventana(popup, titulo);

    /* Color del mensaje */
    if (es_error)
        wattron(popup, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    else
        wattron(popup, COLOR_PAIR(COLOR_OK) | A_BOLD);

    mvwprintw(
        popup,
        3,
        (ancho - (int)strlen(mensaje)) / 2,
        "%s",
        mensaje
    );

    if (es_error)
        wattroff(popup, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    else
        wattroff(popup, COLOR_PAIR(COLOR_OK) | A_BOLD);

    /* Leyenda */
    wattron(popup, COLOR_PAIR(COLOR_NORMAL) | A_DIM);

    mvwprintw(
        popup,
        6,
        (ancho - 26) / 2,
        "[ ENTER PARA CONTINUAR ]"
    );

    wattroff(popup, COLOR_PAIR(COLOR_NORMAL) | A_DIM);

    wrefresh(popup);

    int ch;
    while ((ch = wgetch(popup)) != '\n' &&
           ch != KEY_ENTER)
    {
    }

    /* Limpiar SOLO el popup */
    werase(popup);
    wrefresh(popup);
    delwin(popup);
}


/* Muestra un mensaje en la línea inferior de la pantalla */
void mostrar_mensaje(const char *msg, int es_error) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);
    move(filas - 2, 0);
    clrtoeol();
    if (es_error)
        attron(COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    else
        attron(COLOR_PAIR(COLOR_OK) | A_BOLD);
    mvprintw(filas - 2, 2, "[SYS] %s", msg); // Prefijo estilo terminal
    if (es_error)
        attroff(COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    else
        attroff(COLOR_PAIR(COLOR_OK) | A_BOLD);
    refresh();
}

/* Espera que el usuario pulse ENTER */
void esperar_enter(void) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);
    attron(COLOR_PAIR(COLOR_NORMAL) | A_DIM);
    mvprintw(filas - 1, 2, "> PRESS [ENTER] TO CONTINUE...");
    attroff(COLOR_PAIR(COLOR_NORMAL) | A_DIM);
    refresh();
    /* Consumir hasta ENTER */
    int c;
    while ((c = getch()) != '\n' && c != KEY_ENTER);
}

/* Lee una cadena de texto dentro de una ventana (línea, col, máx) */
int leer_cadena(WINDOW *w, int fila, int col, char *buf, int max) {
    echo(); curs_set(1);
    wmove(w, fila, col); wclrtoeol(w);
    wattron(w, COLOR_PAIR(COLOR_NORMAL));
    wgetnstr(w, buf, max - 1);
    wattroff(w, COLOR_PAIR(COLOR_NORMAL));
    noecho(); curs_set(0);

    // Limpiamos el buffer de posibles saltos de línea y verificamos cancelación
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
            mvwaddch(w, fila, col + pos - 1, '#'); // Estilo Matrix
        }
    }
    
    curs_set(0);
    return 1;
}
void dibujar_leyenda(const char *texto) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Limpiar la última fila completa
    move(max_y - 1, 0); 
    clrtoeol();

    // Aplicar estilo e imprimir centrado
    attron(COLOR_PAIR(COLOR_TITULO) | A_DIM);
    mvprintw(max_y - 1, (max_x - (int)strlen(texto)) / 2, "%s", texto);
    attroff(COLOR_PAIR(COLOR_TITULO) | A_DIM);
    
    refresh(); 
}



/*
  ================== MENÚ GENÉRICO (Estilo Matrix)
  */
int menu_navegar(WINDOW *w, const char **opciones, int n, const char *titulo) {
    int selec = 0;
    int tecla;
    int max_y, max_x;
    
    // Obtenemos dimensiones de la pantalla principal
    getmaxyx(stdscr, max_y, max_x);

    while (1) {
        // 1. Dibujar estructura de la ventana
        dibujar_ventana(w, titulo);

        // 2. Imprimir opciones dentro de la ventana 'w'
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
        wrefresh(w); // Refrescamos solo la ventana del menú

        // 3. Leyenda fija en la última línea de la terminal (stdscr)
        move(max_y - 1, 0); 
        clrtoeol(); // Limpiamos cualquier residuo previo
        
        attron(COLOR_PAIR(COLOR_TITULO) | A_DIM);
        char *texto = "ENTER: Seleccionar | FLECHAS: Navegar | ESC: Salir";
        mvprintw(max_y - 1, (max_x - (int)strlen(texto)) / 2, "%s", texto);
        attroff(COLOR_PAIR(COLOR_TITULO) | A_DIM);
        
        refresh(); // Refrescamos la pantalla completa

        // 4. Captura única de teclas
        tecla = wgetch(w);

        if (tecla == KEY_RESIZE) {
            getmaxyx(stdscr, max_y, max_x); // Actualizamos límites tras redimensionar
            return KEY_RESIZE;
        }
        else if (tecla == KEY_UP) selec = (selec - 1 + n) % n;
        else if (tecla == KEY_DOWN) selec = (selec + 1) % n;
        else if (tecla == 10 || tecla == KEY_ENTER) return selec;
        else if (tecla == 27) return -1; // ESC para salir
    }
}

void banner(void) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x); // Obtiene el tamaño actual de la pantalla

    attron(COLOR_PAIR(COLOR_TITULO)| A_BOLD);

    mvprintw(2, (max_x - 43) / 2, "  ____    _    __  __  _____ ____  _   _ __  __ ");
    mvprintw(3, (max_x - 43) / 2, " / ___|  / \\  |  \\/  || ____| __ )| | | |\\ \\/ / ");
    mvprintw(4, (max_x - 43) / 2, "| |  _  / _ \\ | |\\/| ||  _| |  _ \\| | | | \\  /  ");
    mvprintw(5, (max_x - 43) / 2, "| |_| |/ ___ \\| |  | || |___| |_) | |_| | / / ");
    mvprintw(6, (max_x - 43) / 2, " \\____/_/   \\_\\_|  |_||_____|____/ \\___/ /_/ ");
    
    mvprintw(8, 4, "<");
    mvhline(8, 5, '-', max_x - 10);
    mvprintw(8, max_x - 5, ">");
    
    refresh();
}

/* 
 ================  PANTALLA: Menú inicial (Login / Registro / Salir)
  */
int pantalla_menu_inicial(void) {
    int r = KEY_RESIZE; // Iniciamos en KEY_RESIZE para forzar el primer dibujado
    int filas, cols;
    int alto = 10, ancho = 40;

    // Este bucle solo se repetirá si el usuario redimensiona la ventana
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

        const char *opciones[] = {"[LOG] INICIAR SESION", "[REG] REGISTRARSE", "[EXIT] SALIR DEL SISTEMA"};
        
        // Aquí pasamos el control al menú de navegación
        r = menu_navegar(w, opciones, 3, "VIDEOJUEGOS");
        
        delwin(w); // Limpiamos la ventana antes de volver a dibujar
    }
    return r; // Retorna la opción elegida (0, 1, 2) cuando el usuario presiona ENTER
}
/* 
  ====================== PANTALLA: Formulario de Login / Registro (Estilo Matrix)
   */
void pantalla_login(char *usuario, char *password, char *correo, int es_registro) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    int alto = es_registro ? 14 : 12, ancho = 50;
    int y0   = (filas - alto) / 2;
    int x0   = (cols  - ancho) / 2;

    WINDOW *w = newwin(alto, ancho, y0, x0);
    keypad(w, TRUE);

    const char *titulo = es_registro ? "REGISTRO" : "LOGIN";
    dibujar_ventana(w, titulo);

    wattron(w, COLOR_PAIR(COLOR_TITULO));
    mvwprintw(w, 2, 2, "NOMBRE >");
    wattroff(w, COLOR_PAIR(COLOR_TITULO));
    leer_cadena(w, 3, 2, usuario, 50);

    wattron(w, COLOR_PAIR(COLOR_TITULO));
    mvwprintw(w, 5, 2, "CONTRASENA >");
    wattroff(w, COLOR_PAIR(COLOR_TITULO));
    leer_password(w, 6, 2, password, 100);

    if (es_registro) {
        wattron(w, COLOR_PAIR(COLOR_TITULO));
        mvwprintw(w, 8, 2, "CORREO >");
        wattroff(w, COLOR_PAIR(COLOR_TITULO));
        leer_cadena(w, 9, 2, correo, 70);
    }

    delwin(w);
    clear();
    refresh();
}
/* 
  ==================== PANTALLA: Menú de usuario normal (Estilo Matrix)
   */
int pantalla_menu_usuario(const char *nombre) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    /* Encabezado: Estilo Terminal Hacking */
    attron(COLOR_PAIR(COLOR_TITULO) | A_BOLD);
    mvprintw(1, (cols - 30) / 2, ">> BIENVENID@: %s <<", nombre);
    attroff(COLOR_PAIR(COLOR_TITULO) | A_BOLD);
    refresh();

    int alto = 14, ancho = 40;
    int y0   = (filas - alto) / 2;
    int x0   = (cols  - ancho) / 2;

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
    return r; /* 0-5 */
}


/* 
  =================== PANTALLA: Menú de administrador
   */
int pantalla_menu_admin(void) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    int alto = 20, ancho = 44;
    int y0   = (filas - alto) / 2;
    int x0   = (cols  - ancho) / 2;

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
    return r; /* 0-12 */
}
/* 
  ================= PANTALLA: Catálogo de productos (Estilo Matrix)
   */
int pantalla_catalogo(struct Productos *lista, int total, int *cantidad_out) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    /* Ventana de lista */
    int alto = total + 7, ancho = 72;
    if (alto > filas - 4) alto = filas - 4;
    int y0 = (filas - alto) / 2;
    int x0 = (cols  - ancho) / 2;

    WINDOW *w = newwin(alto, ancho, y0, x0);
    keypad(w, TRUE);
    dibujar_ventana(w, "CATALOGO");

    /* Encabezado de columnas: Estilo tabla de datos */
    wattron(w, COLOR_PAIR(COLOR_BORDE) | A_BOLD);
    mvwprintw(w, 1, 2, "ID  %-28s %-14s  PRECIO    DISPONIBLE", "NOMBRE", "PLATAFORMA");
    wattroff(w, COLOR_PAIR(COLOR_BORDE) | A_BOLD);

    int selec = 0;
    while (1) {
        for (int i = 0; i < total; i++) {
            if (i == selec) wattron(w, COLOR_PAIR(COLOR_SELEC) | A_BOLD);
            else            wattron(w, COLOR_PAIR(COLOR_NORMAL));

            mvwprintw(w, 2 + i, 2, "%-3d %-28.28s %-14.14s $%7.2f  %3d",
                      lista[i].id, lista[i].nombre, lista[i].plataforma,
                      lista[i].precio, lista[i].stock);

            if (i == selec) wattroff(w, COLOR_PAIR(COLOR_SELEC) | A_BOLD);
            else            wattroff(w, COLOR_PAIR(COLOR_NORMAL));
        }
        dibujar_leyenda("ENTER: Seleccionar | FLECHAS: Navegar | ESC: Salir");
        wrefresh(w);

        int tecla = wgetch(w);
        if (tecla == KEY_UP)   selec = (selec - 1 + total) % total;
        else if (tecla == KEY_DOWN) selec = (selec + 1) % total;
        else if (tecla == 'q' || tecla == 27) { delwin(w); clear(); refresh(); return -1; }
        else if (tecla == '\n' || tecla == KEY_ENTER) {
            /* Pedir cantidad en ventana pequeña */
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

    popup_mensaje(
        "ERROR",
        "La cantidad debe ser mayor a cero",
        1
    );
    touchwin(w);
wrefresh(w);
}
else if (cant > lista[selec].stock) {

    popup_mensaje(
        "ERROR",
        "No hay suficiente stock disponible",
        1
    );
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

/* 
 ====================  PANTALLA: Carrito de compras (Estilo Matrix)
   */
int pantalla_carrito(struct Productos *carrito, int total, float costo_total) {
    int filas, cols;
    getmaxyx(stdscr, filas, cols);

    int alto = total + 9, ancho = 68;
    if (alto > filas - 4) alto = filas - 4;
    int y0 = (filas - alto) / 2;
    int x0 = (cols  - ancho) / 2;

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

    /* Confirmación con mini-menú */
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

/* 
   ======================PANTALLA: Visualización de texto largo (historial, reportes, etc.)
*/
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
            // Antes de limpiar, apagamos el color
            wattroff(w, COLOR_PAIR(COLOR_NORMAL));
            
            mvwprintw(w, alto - 2, 2, "-- Más abajo --");
            wrefresh(w);
            wgetch(w);
            wclear(w);
            dibujar_ventana(w, titulo);
            fila_actual = 2;
        }

        // Activamos el color ANTES de imprimir cada línea
        wattron(w, COLOR_PAIR(COLOR_NORMAL));
        mvwprintw(w, fila_actual++, 2, "%s", linea);
        wattroff(w, COLOR_PAIR(COLOR_NORMAL));
        
        linea = strtok(NULL, "\n");
    }

    // Pie de página con color
    wattron(w, COLOR_PAIR(COLOR_NORMAL));
    mvwprintw(w, alto - 1, 2, "Presione cualquier tecla para regresar...");
    wattroff(w, COLOR_PAIR(COLOR_NORMAL));
    
    wrefresh(w);
    wgetch(w);
    delwin(w);
    clear();
    refresh();
}
/* 
   ===============PANTALLA: Formulario genérico (Estilo Matrix)
*/
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
        /* Estilo prompt de sistema */
        wattron(w, COLOR_PAIR(COLOR_TITULO));
        mvwprintw(w, 2 + i * 3, 2, "> %s:", campos[i]);
        wattroff(w, COLOR_PAIR(COLOR_TITULO));
        
        /* Instrucción de salida discreta */
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
    clear(); refresh();
    return exito;
}

/* 
 ===============  MAIN================
*/



int validar_password(const char *pass, char *error)
{
    int mayus = 0;
    int minus = 0;
    int numero = 0;

    if(strlen(pass) < 8)
    {
        strcpy(error,
               "Minimo 8 caracteres");
        return 0;
    }

    for(int i = 0; pass[i] != '\0'; i++)
    {
        if(isupper(pass[i]))
            mayus = 1;

        if(islower(pass[i]))
            minus = 1;

        if(isdigit(pass[i]))
            numero = 1;
    }

    if(!mayus)
    {
        strcpy(error,
               "Debe contener una mayuscula");
        return 0;
    }

    if(!minus)
    {
        strcpy(error,
               "Debe contener una minuscula");
        return 0;
    }

    if(!numero)
    {
        strcpy(error,
               "Debe contener un numero");
        return 0;
    }

    return 1;
}
int main(void) {



    /* ── Semáforos y memoria compartida ──────────────────────── */
    key_t llave        = ftok("archivo", 'k');
    key_t llave_cliente = ftok("archivo", 'c');

    if (llave == -1 || llave_cliente == -1) {
        fprintf(stderr, "Error al crear la llave\n");
        exit(1);
    }

    int semaforo        = semget(llave,        1, PERMISOS);
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

    /* ── Inicializar ncurses ──────────────────────────────────── */
    initscr();
    start_color();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);

    /* Definir pares de color */
    inicializar_colores();

    int sesion_iniciada = 0;
    char usuario_actual[50] = {0};
    int  mi_rol_admin       = 0;

    /* 
       ====================BUCLE MENÚ INICIAL (Login / Registro)
    */
    while (!sesion_iniciada)
{
    int opcion_ini = pantalla_menu_inicial();

    if (opcion_ini == 2 || opcion_ini == -1)
    {
        endwin();
        return 0;
    }

    int es_registro = (opcion_ini == 1);

    /* =========================
       REGISTRO
       ========================= */
    if (es_registro)
    {
        while (1)
        {
            char usuario_local[50] = {0};
            char password_local[100] = {0};
            char correo_local[70] = {0};
            char password_cifrada[100] = {0};

            pantalla_login(
                usuario_local,
                password_local,
                correo_local,
                1
            );
            char error_pass[100];

            if (!validar_password(password_local,
                                  error_pass))
            {
                popup_mensaje(
                    "PASSWORD",
                    error_pass,
                    1);

                continue;
            }
            contrasenacif(
                password_local,
                password_cifrada
            );

            memoria->accion = REGISTRO;

            strncpy(memoria->usuario,
                    usuario_local,
                    49);

            strncpy(memoria->password,
                    password_cifrada,
                    99);

            strncpy(memoria->correo,
                    correo_local,
                    69);

            up(semaforo);
            down(semaforo_cliente);

            if (strcmp(memoria->respuesta,
                       "Registro exitoso") == 0)
            {
                popup_mensaje(
                    "REGISTRO",
                    memoria->respuesta,
                    0
                );

                break;
            }

            popup_mensaje(
                "ERROR",
                memoria->respuesta,
                1
            );
        }

        continue;
    }

    /* =========================
       LOGIN
       ========================= */
    char usuario_local[50] = {0};
    char password_local[100] = {0};
    char correo_local[70] = {0};
    char password_cifrada[100] = {0};

    pantalla_login(
        usuario_local,
        password_local,
        correo_local,
        0
    );

    contrasenacif(
        password_local,
        password_cifrada
    );

    memoria->accion = LOGIN;

    strncpy(memoria->usuario,
            usuario_local,
            49);

    strncpy(memoria->password,
            password_cifrada,
            99);

    up(semaforo);
    down(semaforo_cliente);

    if (strcmp(memoria->respuesta,
               "Login exitoso") == 0)
    {
        sesion_iniciada = 1;

        mi_rol_admin = memoria->es_admin;

        strncpy(usuario_actual,
                usuario_local,
                49);

        popup_mensaje(
            "LOGIN",
            "Bienvenid@",
            0
        );
    }
    else
    {
        popup_mensaje(
            "ERROR",
            memoria->respuesta,
            1
        );
    }
}

    /* 
       ==================BUCLE MENÚ PRINCIPAL (usuario / admin)
     */
    while (sesion_iniciada) {

        int opcion = mi_rol_admin
                     ? pantalla_menu_admin()
                     : pantalla_menu_usuario(usuario_actual);

        if (opcion == -1) continue; /* tecla 'q', volver al menú */

        /* ── Cerrar sesión ─────────────────────────────────── */
        int opcion_cerrar = mi_rol_admin ? 12 : 5;
        if (opcion == opcion_cerrar) {
            sesion_iniciada = 0;
            mi_rol_admin    = 0;
            mostrar_mensaje("Sesion cerrada.", 0);
            esperar_enter();
            /* Volver al menú inicial */
            break;
        }

        /* 
           ==============OPCIONES USUARIO
         */

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
    strstr(memoria->respuesta, "ERROR") != NULL ?
        "ERROR" : "CARRITO",
    memoria->respuesta,
    strstr(memoria->respuesta, "ERROR") != NULL
);
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
                    sscanf(linea,
                    "%d;%[^;];%[^;];%f;%d",
                    &carrito[total].id,
                    carrito[total].nombre,
                    carrito[total].plataforma,
                    &carrito[total].precio,
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

            popup_mensaje(
                "COMPRA",
                memoria->respuesta,
                0
            );
        }
    }

    else if (strcmp(memoria->respuesta, "VACIO") == 0) {

        popup_mensaje(
            "AVISO",
            "Tu carrito esta vacio",
            1
        );
    }

    else {

        popup_mensaje(
            "ERROR",
            memoria->respuesta,
            1
        );
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
                { mostrar_mensaje("Aun no tienes compras.", 1); esperar_enter(); }
            else
                { mostrar_mensaje(memoria->respuesta, 1); esperar_enter(); }
        }

        /* [3] Buscar producto (usuario) */
        /* [3] Buscar producto (usuario estándar) */
        else if (!mi_rol_admin && opcion == 3) {
            const char *campos[] = {"Nombre del producto"};
            char buf[200] = {0};
            char *vals[]  = {buf};
            const int maxs[] = {199};
            const int pass[] = {0};

            // Verificamos si el formulario se completó exitosamente
            if (pantalla_formulario("BUSCAR PRODUCTO", campos, vals, maxs, pass, 1)) {
                
                strncpy(memoria->productos, buf, 199);
                memoria->accion = BUSCAR_PRODUCTO;
                
                up(semaforo);
                down(semaforo_cliente);
                
                // Mostrar resultados solo si la operación fue confirmada
                pantalla_texto("RESULTADOS DE BUSQUEDA", memoria->productos);
                
            } else {
                mostrar_mensaje("Búsqueda cancelada.", 1);
            }

            clear();
            refresh();
        }

        /* [4] Modificar mi cuenta */
       
else if (!mi_rol_admin && opcion == 4) {

    const char *campos[] = {
        "Nuevo usuario",
        "Nueva password",
        "Nuevo correo"
    };

    char nu[50]={0},
         np[100]={0},
         nc[70]={0};

    char *vals[] = {nu, np, nc};

    const int maxs[] = {49, 99, 69};
    const int pass[] = {0, 1, 0};

    pantalla_formulario(
        "MODIFICAR MI CUENTA",
        campos,
        vals,
        maxs,
        pass,
        3
    );

    char np_cif[100] = {0};

    contrasenacif(np, np_cif);

    strncpy(memoria->usuario,  nu,     49);
    strncpy(memoria->password, np_cif, 99);
    strncpy(memoria->correo,   nc,     69);
    strncpy(memoria->mensaje,  usuario_actual, 49);

    memoria->accion = MODIFICAR_MI_CUENTA;

    up(semaforo);
    down(semaforo_cliente);

    if (strstr(memoria->respuesta, "ERROR") == NULL)
    {
        strncpy(usuario_actual,
                memoria->usuario,
                49);
    }

    popup_mensaje(
        strstr(memoria->respuesta, "ERROR") != NULL ?
            "ERROR" : "MI CUENTA",
        memoria->respuesta,
        strstr(memoria->respuesta, "ERROR") != NULL
    );
}

        /* 
          ========================= OPCIONES ADMINISTRADOR
          */

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
                /* Mostrar sin opción de agregar al carrito */
                int dummy;
                pantalla_catalogo(lista, total, &dummy);
            }
        }

        /* [1] Agregar producto */
        else if (mi_rol_admin && opcion == 1) {
    const char *campos[] = {"ID", "Nombre", "Plataforma", "Precio", "Stock"};
    char sid[10]={0}, nom[50]={0}, pla[50]={0}, pre[20]={0}, sto[10]={0};
    char *vals[]    = {sid, nom, pla, pre, sto};
    const int maxs[]= {9, 49, 49, 19, 9};
    const int pass[]= {0,0,0,0,0};

    // Llamada modificada: El 'if' verifica si el formulario terminó con éxito (1) o fue cancelado (0)
    if (pantalla_formulario("AGREGAR PRODUCTO", campos, vals, maxs, pass, 5)) {
        
        memoria->producto.id     = atoi(sid);
        strncpy(memoria->producto.nombre,     nom, 49);
        strncpy(memoria->producto.plataforma, pla, 49);
        memoria->producto.precio = atof(pre);
        memoria->producto.stock  = atoi(sto);

        memoria->accion = AGREGAR_PRODUCTO_ADMIN;
        up(semaforo);
        down(semaforo_cliente);
        
        mostrar_mensaje(memoria->respuesta, strstr(memoria->respuesta, "ERROR") != NULL);
        esperar_enter();
        
    } else {
        // Esto se ejecuta si el usuario escribió 'q' y presionó Enter
        mostrar_mensaje("Operación cancelada por el usuario.", 0);
    }
}

        /* [2] Modificar stock */
        else if (mi_rol_admin && opcion == 2) {
            const char *campos[] = {"ID del producto", "Nuevo stock"};
            char sid[10]={0}, sst[10]={0};
            char *vals[]    = {sid, sst};
            const int maxs[]= {9, 9};
            const int pass[]= {0, 0};

            // Envolvemos en un IF para detectar si el usuario canceló con 'q'
            if (pantalla_formulario("MODIFICAR STOCK", campos, vals, maxs, pass, 2)) {
                
                memoria->producto.id    = atoi(sid);
                memoria->producto.stock = atoi(sst);
                memoria->accion = MODIFICAR_STOCK;
                
                up(semaforo);
                down(semaforo_cliente);
                
                mostrar_mensaje(memoria->respuesta, strstr(memoria->respuesta, "ERROR") != NULL);
                esperar_enter();
                
            } else {
                // Se ejecuta si el usuario presionó 'q'
                mostrar_mensaje("Modificación de stock cancelada.", 1);
            }
            
            // Limpiamos pantalla tras salir del formulario
            clear(); 
            refresh();
        }

        /* [3] Eliminar producto */
        else if (mi_rol_admin && opcion == 3) {
            const char *campos[] = {"ID del producto a eliminar"};
            char sid[10]={0};
            char *vals[]    = {sid};
            const int maxs[]= {9};
            const int pass[]= {0};

            // Verificamos si el formulario se completó exitosamente
            if (pantalla_formulario("ELIMINAR PRODUCTO", campos, vals, maxs, pass, 1)) {
                
                memoria->producto.id = atoi(sid);
                memoria->accion      = ELIMINAR_PRODUCTO;
                
                up(semaforo);
                down(semaforo_cliente);
                
                mostrar_mensaje(memoria->respuesta, strstr(memoria->respuesta, "ERROR") != NULL);
                esperar_enter();
                
            } else {
                mostrar_mensaje("Eliminación cancelada.", 1);
            }

            clear();
            refresh();
        }
/* [4] Buscar producto (admin) */
        else if (mi_rol_admin && opcion == 4) {
            const char *campos[] = {"Nombre del producto"};
            char buf[200] = {0};
            char *vals[]  = {buf};
            const int maxs[]= {199};
            const int pass[]= {0};

            // Verificamos si el formulario se completó
            if (pantalla_formulario("BUSCAR PRODUCTO", campos, vals, maxs, pass, 1)) {
                
                strncpy(memoria->productos, buf, 199);
                memoria->accion = BUSCAR_PRODUCTO;
                
                up(semaforo);
                down(semaforo_cliente);
                
                // Mostramos resultados solo si el usuario no canceló
                pantalla_texto("RESULTADOS", memoria->productos);
                
            } else {
                mostrar_mensaje("Búsqueda cancelada.", 1);
                esperar_enter();
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
            char nu[50]={0}, np[100]={0}, nc[70]={0};
            char *vals[]    = {nu, np, nc};
            const int maxs[]= {49, 99, 69};
            const int pass[]= {0, 1, 0};

            // Verificamos si el formulario se completó exitosamente
            if (pantalla_formulario("CREAR USUARIO", campos, vals, maxs, pass, 3)) {
                
                char np_cif[100] = {0};
                contrasenacif(np, np_cif);
                
                strncpy(memoria->usuario,  nu,     49);
                strncpy(memoria->password, np_cif, 99);
                strncpy(memoria->correo,   nc,     69);
                
                memoria->accion = CREAR_USUARIO_ADMIN;
                up(semaforo);
                down(semaforo_cliente);
                
                mostrar_mensaje(memoria->respuesta, strstr(memoria->respuesta, "ERROR") != NULL);
                esperar_enter();
                
            } else {
                mostrar_mensaje("Creación de usuario cancelada.", 1);
            }

            clear();
            refresh();
        }

        /* [7] Modificar usuario */
        else if (mi_rol_admin && opcion == 7) {
            const char *campos[] = {"Usuario a modificar", "Nuevo usuario", "Nueva password", "Nuevo correo"};
            char vm[50]={0}, nu[50]={0}, np[100]={0}, nc[70]={0};
            char *vals[]    = {vm, nu, np, nc};
            const int maxs[]= {49, 49, 99, 69};
            const int pass[]= {0, 0, 1, 0};

            // Verificamos si el formulario se completó exitosamente
            if (pantalla_formulario("MODIFICAR USUARIO", campos, vals, maxs, pass, 4)) {
                
                char np_cif[100] = {0};
                contrasenacif(np, np_cif);
                
                strncpy(memoria->mensaje,  vm,     49);
                strncpy(memoria->usuario,  nu,     49);
                strncpy(memoria->password, np_cif, 99);
                strncpy(memoria->correo,   nc,     69);
                
                memoria->accion = MODIFICAR_USUARIO;
                up(semaforo);
                down(semaforo_cliente);
                
                mostrar_mensaje(memoria->respuesta, strstr(memoria->respuesta, "ERROR") != NULL);
                esperar_enter();
                
            } else {
                mostrar_mensaje("Modificación de usuario cancelada.", 1);
            }

            clear();
            refresh();
        }
/* [8] Eliminar usuario */
        else if (mi_rol_admin && opcion == 8) {
            const char *campos[] = {"Usuario a eliminar"};
            char buf[50] = {0};
            char *vals[]      = {buf};
            const int maxs[] = {49};
            const int pass[] = {0};

            // Verificamos si el formulario se completó exitosamente
            if (pantalla_formulario("ELIMINAR USUARIO", campos, vals, maxs, pass, 1)) {
                
                strncpy(memoria->mensaje, buf, 49);
                memoria->accion = ELIMINAR_USUARIO;
                
                up(semaforo);
                down(semaforo_cliente);
                
                mostrar_mensaje(memoria->respuesta, strstr(memoria->respuesta, "ERROR") != NULL);
                esperar_enter();
                
            } else {
                mostrar_mensaje("Eliminación de usuario cancelada.", 1);
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

    /* Si cerró sesión, volver al menú inicial recursivamente */
    endwin();
    return main();   /* reinicia sin reabrir la conexión */
}