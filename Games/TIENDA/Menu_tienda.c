#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>

void mostrar_catalogo(int carrito_ids[], int *total_carrito);
void ver_carrito(int carrito_ids[], int total_carrito);
void ver_historial();
void agregar_saldo();

// Definimos la estructura para almacenar los datos de cada videojuego
struct Videojuego {
    int id;
    char titulo[30];
    char plataforma[15];
    float precio;
    int stock;
};





void abrir_menu_tienda(const char *usuario_autenticado) {
    int max_y, max_x;
    int selecionado = 0;
    int tecla;
    int ejecutando = 1;

    // --- VARIABLES DEL CARRITO ---
    int carrito_ids[100];
    int total_carrito = 0;

    char *opciones[5] = {
        "[1] Ver catálogo de juegos",
        "[2] Ver carrito de compras",
        "[3] Historial de compras",
        "[4] Agregar saldo a la cuenta",
        "[5] Cerrar sesión"
    };

    while (ejecutando) {
        getmaxyx(stdscr, max_y, max_x);
        clear();

        int ancho_caja = 50;
        int alto_caja = 13;
        int inicio_y = (max_y - alto_caja) / 2;
        int inicio_x = (max_x - ancho_caja) / 2;

        mvprintw(inicio_y, inicio_x, "+------------------------------------------------+");
        for (int i = 1; i < alto_caja - 1; i++) {
            mvprintw(inicio_y + i, inicio_x, "|");
            mvprintw(inicio_y + i, inicio_x + ancho_caja - 1, "|");
        }
        mvprintw(inicio_y + alto_caja - 1, inicio_x, "+------------------------------------------------+");

        mvprintw(inicio_y + 1, inicio_x + 3, "Bienvenido a la Tienda, %s!", usuario_autenticado);
        mvprintw(inicio_y + 2, inicio_x + 3, "Tienda de videojuegos");
        mvprintw(inicio_y + 3, inicio_x, "|------------------------------------------------|");

        mvprintw(inicio_y + 5, inicio_x + 6, "%s %s", (selecionado == 0) ? "->" : "  ", opciones[0]);
        mvprintw(inicio_y + 6, inicio_x + 6, "%s %s", (selecionado == 1) ? "->" : "  ", opciones[1]);
        mvprintw(inicio_y + 7, inicio_x + 6, "%s %s", (selecionado == 2) ? "->" : "  ", opciones[2]);
        mvprintw(inicio_y + 8, inicio_x + 6, "%s %s", (selecionado == 3) ? "->" : "  ", opciones[3]);
        mvprintw(inicio_y + 9, inicio_x + 6, "%s %s", (selecionado == 4) ? "->" : "  ", opciones[4]);
        
        attron(COLOR_PAIR(2));
        mvhline(max_y - 1, 0, ' ', max_x);
        char *leyenda = "ENTER: Seleccionar | FLECHAS: Navegar";
        mvprintw(max_y - 1, (max_x - strlen(leyenda)) / 2, "%s", leyenda);
        attroff(COLOR_PAIR(2));
        refresh();

        tecla = getch();

        if (tecla == KEY_UP && selecionado > 0) {
            selecionado--;
        }
        else if (tecla == KEY_DOWN && selecionado < 4) {
            selecionado++;
        }

//////// conectamos las opciones con las funciones para entrar a cada menu
        
        else if (tecla == 10) { 
            if (selecionado == 0) { 
                mostrar_catalogo(carrito_ids, &total_carrito); 
            }
            else if (selecionado == 1) { 
                ver_carrito(carrito_ids, total_carrito); 
            }
            else if (selecionado == 2) { 
                ver_historial(); 
            }
            else if (selecionado == 3) { 
                agregar_saldo(); 
            }
            else if (selecionado == 4) { 
                ejecutando = 0; 
            }
        }
    }
}


void mostrar_catalogo(int carrito_ids[], int *total_carrito) {
    int max_y, max_x;
    int tecla;
    int ejecutando = 1;
    int seleccionado_juego = 0; 

    struct Videojuego lista_juegos[100];
    int total_juegos = 0;

    FILE *archivo = fopen("juegos.txt", "r");
    if (archivo == NULL) {
        clear();
        mvprintw(2, 5, "Error: No se pudo abrir 'juegos.txt'");
        getch();
        return;
    }

    while (fscanf(archivo, "%d,%[^,],%[^,],%f,%d\n", 
                  &lista_juegos[total_juegos].id, 
                  lista_juegos[total_juegos].titulo, 
                  lista_juegos[total_juegos].plataforma, 
                  &lista_juegos[total_juegos].precio, 
                  &lista_juegos[total_juegos].stock) != EOF) {
        total_juegos++;
        if (total_juegos >= 100) break;
    }
    fclose(archivo);

    while (ejecutando) {
        getmaxyx(stdscr, max_y, max_x);
        clear();

        attron(A_BOLD);
        mvprintw(2, 5, "ID   %-25s %-12s %-10s %s", "GAME TITLE", "PLATFORM", "PRICE", "STOCK");
        attroff(A_BOLD);
        
        mvhline(3, 5, '=', max_x - 10);

        for (int i = 0; i < total_juegos; i++) {
            if (i == seleccionado_juego) {
                attron(COLOR_PAIR(2) | A_BOLD); 
            }

            mvprintw(5 + i, 5, "%-4d %-25s %-12s $%-9.2f %-15d", 
                     lista_juegos[i].id, 
                     lista_juegos[i].titulo, 
                     lista_juegos[i].plataforma, 
                     lista_juegos[i].precio, 
                     lista_juegos[i].stock);

            if (i == seleccionado_juego) {
                attroff(COLOR_PAIR(2) | A_BOLD);
            }
        }

        attron(COLOR_PAIR(2));
        mvhline(max_y - 1, 0, ' ', max_x);
        char *leyenda = "[ENTER] Agregar al Carrito | [UP/DOWN] Select | [ESC] Back";
        mvprintw(max_y - 1, (max_x - strlen(leyenda)) / 2, "%s", leyenda);
        attroff(COLOR_PAIR(2));

        refresh();
        tecla = getch();

        if (tecla == KEY_UP && seleccionado_juego > 0) {
            seleccionado_juego--;
        }
        else if (tecla == KEY_DOWN && seleccionado_juego < total_juegos - 1) {
            seleccionado_juego++;
        }
        else if (tecla == 10) { // ENTER: Agrega el juego seleccionado al carrito
            if (*total_carrito < 100) {
                carrito_ids[*total_carrito] = lista_juegos[seleccionado_juego].id;
                (*total_carrito)++;
                
                // Pequeño aviso visual de agregado
                mvprintw(max_y - 2, 5, "[ OK ] ¡%s agregado al carrito!", lista_juegos[seleccionado_juego].titulo);
                refresh();
                napms(600); // Pausa corta de 600 milisegundos sin congelar el buffer de ncurses
            }
        }
        else if (tecla == 27) { 
            ejecutando = 0;
        }
    }
}


void ver_carrito(int carrito_ids[], int total_carrito) {
    int max_y, max_x;
    int tecla;
    int ejecutando = 1;
    int seleccionado_carrito = 0;

    // Cargamos la base de datos de juegos completa para contrastar los IDs
    struct Videojuego base_juegos[100];
    int total_base = 0;

    FILE *archivo = fopen("juegos.txt", "r");
    if (archivo != NULL) {
        while (fscanf(archivo, "%d,%[^,],%[^,],%f,%d\n", 
                      &base_juegos[total_base].id, 
                      base_juegos[total_base].titulo, 
                      base_juegos[total_base].plataforma, 
                      &base_juegos[total_base].precio, 
                      &base_juegos[total_base].stock) != EOF) {
            total_base++;
        }
        fclose(archivo);
    }

    while (ejecutando) {
        getmaxyx(stdscr, max_y, max_x);
        clear();

        // --- ENCABEZADOS DEL CARRITO ---
        attron(A_BOLD);
        mvprintw(2, 5, "CARRITO DE COMPRAS (Articulos: %d)", total_carrito);
        mvprintw(4, 5, "ID   %-25s %-12s %-10s", "GAME TITLE", "PLATFORM", "PRICE");
        attroff(A_BOLD);
        
        mvhline(5, 5, '=', max_x - 10);

        float total_pagar = 0.0;

        // --- RENDERIZADO DEL CARRITO ---
        if (total_carrito == 0) {
            mvprintw(7, 5, "Tu carrito esta vacio.");
        } else {
            for (int i = 0; i < total_carrito; i++) {
                // Buscamos los datos completos del juego usando el ID guardado
                struct Videojuego juego_actual;
                for (int j = 0; j < total_base; j++) {
                    if (base_juegos[j].id == carrito_ids[i]) {
                        juego_actual = base_juegos[j];
                        break;
                    }
                }

                total_pagar += juego_actual.precio;

                if (i == seleccionado_carrito) {
                    attron(COLOR_PAIR(2) | A_BOLD);
                }

                mvprintw(7 + i, 5, "%-4d %-25s %-12s $%-9.2f", 
                         juego_actual.id, 
                         juego_actual.titulo, 
                         juego_actual.plataforma, 
                         juego_actual.precio);

                if (i == seleccionado_carrito) {
                    attroff(COLOR_PAIR(2) | A_BOLD);
                }
            }
        }

        // Mostrar total acumulado abajo de la tabla
        mvhline(7 + total_carrito + 1, 5, '-', 55);
        attron(A_BOLD);
        mvprintw(7 + total_carrito + 2, 5, "TOTAL A PAGAR: $%.2f", total_pagar);
        attroff(A_BOLD);

        // --- BARRA INFERIOR DE GUÍA ---
        attron(COLOR_PAIR(2));
        mvhline(max_y - 1, 0, ' ', max_x);
        char *leyenda = "[UP/DOWN] Navegar | [ESC] Volver al Menu";
        mvprintw(max_y - 1, (max_x - strlen(leyenda)) / 2, "%s", leyenda);
        attroff(COLOR_PAIR(2));

        refresh();
        tecla = getch();

        if (tecla == KEY_UP && seleccionado_carrito > 0) {
            seleccionado_carrito--;
        }
        else if (tecla == KEY_DOWN && seleccionado_carrito < total_carrito - 1) {
            seleccionado_carrito++;
        }
        else if (tecla == 27) { // ESC para volver
            ejecutando = 0;
        }
    }
}


void ver_historial() {
    int max_y, max_x;
    int tecla;
    int ejecutando = 1;
    int seleccionado_historial = 0;

    // 1. Cargar base de datos de juegos para obtener nombres y precios
    struct Videojuego base_juegos[100];
    int total_base = 0;

    FILE *f_juegos = fopen("juegos.txt", "r");
    if (f_juegos != NULL) {
        while (fscanf(f_juegos, "%d,%[^,],%[^,],%f,%d\n", 
                      &base_juegos[total_base].id, 
                      base_juegos[total_base].titulo, 
                      base_juegos[total_base].plataforma, 
                      &base_juegos[total_base].precio, 
                      &base_juegos[total_base].stock) != EOF) {
            total_base++;
        }
        fclose(f_juegos);
    }

    // 2. Cargar los IDs comprados desde historial.txt
    int historial_ids[100];
    int total_historial = 0;

    FILE *f_historial = fopen("historial.txt", "r");
    if (f_historial != NULL) {
        while (fscanf(f_historial, "%d\n", &historial_ids[total_historial]) != EOF) {
            total_historial++;
            if (total_historial >= 100) break;
        }
        fclose(f_historial);
    }

    while (ejecutando) {
        getmaxyx(stdscr, max_y, max_x);
        clear();

        // --- ENCABEZADOS DEL HISTORIAL ---
        attron(A_BOLD);
        mvprintw(2, 5, "HISTORIAL DE COMPRAS (Total articulos: %d)", total_historial);
        mvprintw(4, 5, "ID   %-25s %-12s %-10s", "GAME TITLE", "PLATFORM", "PRICE");
        attroff(A_BOLD);
        
        mvhline(5, 5, '=', max_x - 10);

        float total_invertido = 0.0;

        // --- RENDERIZADO DEL HISTORIAL ---
        if (total_historial == 0) {
            mvprintw(7, 5, "No has realizado ninguna compra todavia.");
        } else {
            for (int i = 0; i < total_historial; i++) {
                struct Videojuego juego_comprado;
                int encontrado = 0;

                // Buscar datos del juego por ID
                for (int j = 0; j < total_base; j++) {
                    if (base_juegos[j].id == historial_ids[i]) {
                        juego_comprado = base_juegos[j];
                        encontrado = 1;
                        break;
                    }
                }

                if (encontrado) {
                    total_invertido += juego_comprado.precio;

                    if (i == seleccionado_historial) {
                        attron(COLOR_PAIR(2) | A_BOLD);
                    }

                    mvprintw(7 + i, 5, "%-4d %-25s %-12s $%-9.2f", 
                             juego_comprado.id, 
                             juego_comprado.titulo, 
                             juego_comprado.plataforma, 
                             juego_comprado.precio);

                    if (i == seleccionado_historial) {
                        attroff(COLOR_PAIR(2) | A_BOLD);
                    }
                }
            }
        }

        // Mostrar total invertido histórico abajo
        mvhline(7 + total_historial + 1, 5, '-', 55);
        attron(A_BOLD);
        mvprintw(7 + total_historial + 2, 5, "TOTAL INVERTIDO EN LA TIENDA: $%.2f", total_invertido);
        attroff(A_BOLD);

        // --- BARRA INFERIOR DE GUÍA ---
        attron(COLOR_PAIR(2));
        mvhline(max_y - 1, 0, ' ', max_x);
        char *leyenda = "[UP/DOWN] Navegar | [ESC] Volver al Menu";
        mvprintw(max_y - 1, (max_x - strlen(leyenda)) / 2, "%s", leyenda);
        attroff(COLOR_PAIR(2));

        refresh();
        tecla = getch();

        if (tecla == KEY_UP && seleccionado_historial > 0) {
            seleccionado_historial--;
        }
        else if (tecla == KEY_DOWN && seleccionado_historial < total_historial - 1) {
            seleccionado_historial++;
        }
        else if (tecla == 27) { // ESC para regresar
            ejecutando = 0;
        }
    }
}

void agregar_saldo() {
    int max_y, max_x;
    float saldo_actual = 0.0;
    float monto_a_agregar = 0.0;
    char entrada_str[10] = "";

    // 1. LEER EL SALDO ACTUAL DESDE EL ARCHIVO
    FILE *f_saldo = fopen("saldo.txt", "r");
    if (f_saldo != NULL) {
        fscanf(f_saldo, "%f", &saldo_actual);
        fclose(f_saldo);
    }

    getmaxyx(stdscr, max_y, max_x);
    clear();

    // 2. DIMENSIONES Y CENTRADO DE LA CAJA DE DIÁLOGO
    int ancho_caja = 54;
    int alto_caja = 11;
    int inicio_y = (max_y - alto_caja) / 2;
    int inicio_x = (max_x - ancho_caja) / 2;

    // Dibujar el marco de la caja
    mvprintw(inicio_y, inicio_x, "+----------------------------------------------------+");
    for (int i = 1; i < alto_caja - 1; i++) {
        mvprintw(inicio_y + i, inicio_x, "|");
        mvprintw(inicio_y + i, inicio_x + ancho_caja - 1, "|");
    }
    mvprintw(inicio_y + alto_caja - 1, inicio_x, "+----------------------------------------------------+");

    // Contenido de la caja
    attron(A_BOLD);
    mvprintw(inicio_y + 2, inicio_x + 4, "--- MONEDERO ELECTRONICO ---");
    attroff(A_BOLD);
    
    mvprintw(inicio_y + 4, inicio_x + 4, "Saldo actual disponible: ");
    attron(COLOR_PAIR(2) | A_BOLD);
    printw("$%.2f", saldo_actual);
    attroff(COLOR_PAIR(2) | A_BOLD);

    mvprintw(inicio_y + 6, inicio_x + 4, "Monto a ingresar: $ ");
    
    // Habilitar temporalmente la visualización de caracteres en la terminal para la entrada
    echo();
    curs_set(1); // Mostrar el cursor parpadeando en el campo de texto

    // Leer el monto como cadena de texto (máximo 7 caracteres para evitar desbordamientos)
    wgetnstr(stdscr, entrada_str, 7);

    // Deshabilitar de nuevo el cursor y el eco para mantener la interfaz limpia
    noecho();
    curs_set(0);

    // Convertir la entrada de texto a un valor flotante
    monto_a_agregar = atof(entrada_str);

    // Validar que el monto sea un número positivo real
    if (monto_a_agregar > 0) {
        saldo_actual += monto_a_agregar;

        // 3. GUARDAR EL NUEVO SALDO EN EL ARCHIVO
        FILE *f_saldo_w = fopen("saldo.txt", "w");
        if (f_saldo_w != NULL) {
            fprintf(f_saldo_w, "%.2f", saldo_actual);
            fclose(f_saldo_w);
        }

        // Mensaje de éxito de la transacción
        mvprintw(inicio_y + 8, inicio_x + 4, "[ OK ] ¡Saldo agregado correctamente!");
    } else {
        // Mensaje en caso de ingresar un valor inválido o cero
        mvprintw(inicio_y + 8, inicio_x + 4, "[ ERR ] Monto invalido. Operacion cancelada.");
    }

    refresh();
    napms(1500); // Pausa de 1.5 segundos para que alcances a leer el resultado en pantalla
}