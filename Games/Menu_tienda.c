#include <ncurses.h>
#include <string.h>

void mostrar_catalogo();

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

    // opciones del menu
    char *opciones[5] = {
        "[1] Ver catálogo de juegos",
        "[2] Ver carrito de compras",
        "[3] Historial de compras",
        "[4] Agregar saldo a la cuenta",
        "[5] Cerrar sesión"
    };

    while (ejecutando) {
        getmaxyx(stdscr, max_y, max_x); // se mide tamaño de pantalla
        clear(); // se limpia la pantalla

        // --- DISEÑO DE LA CAJA CENTRADA ---
        int ancho_caja = 50;
        int alto_caja = 13;
        int inicio_y = (max_y - alto_caja) / 2;
        int inicio_x = (max_x - ancho_caja) / 2;

        // Borde superior
        mvprintw(inicio_y, inicio_x, "+------------------------------------------------+");

        // Paredes laterales de la caja
        for (int i = 1; i < alto_caja - 1; i++) {
            mvprintw(inicio_y + i, inicio_x, "|");
            mvprintw(inicio_y + i, inicio_x + ancho_caja - 1, "|");
        }

        // Borde inferior de la caja
        mvprintw(inicio_y + alto_caja - 1, inicio_x, "+------------------------------------------------+");

        // --- ENCABEZADO DENTRO DE LA CAJA ---
        mvprintw(inicio_y + 1, inicio_x + 3, "Bienvenido a la Tienda, %s!", usuario_autenticado);
        mvprintw(inicio_y + 2, inicio_x + 3, "Tienda de videojuegos");
        mvprintw(inicio_y + 3, inicio_x, "|------------------------------------------------|");

        // --- MOSTRAR OPCIONES DEL MENU DENTRO DE LA CAJA ---
        mvprintw(inicio_y + 5, inicio_x + 6, "%s %s", (selecionado == 0) ? "->" : "  ", opciones[0]);
        mvprintw(inicio_y + 6, inicio_x + 6, "%s %s", (selecionado == 1) ? "->" : "  ", opciones[1]);
        mvprintw(inicio_y + 7, inicio_x + 6, "%s %s", (selecionado == 2) ? "->" : "  ", opciones[2]);
        mvprintw(inicio_y + 8, inicio_x + 6, "%s %s", (selecionado == 3) ? "->" : "  ", opciones[3]);
        mvprintw(inicio_y + 9, inicio_x + 6, "%s %s", (selecionado == 4) ? "->" : "  ", opciones[4]);
        
        // Barra inferior
        attron(COLOR_PAIR(2));
        mvhline(max_y - 1, 0, ' ', max_x);

        char *leyenda = "ENTER: Seleccionar | FLECHAS: Navegar";
        
        // centrar texto de la barra inferior
        mvprintw(max_y - 1, (max_x - strlen(leyenda)) / 2, "%s", leyenda);
        attroff(COLOR_PAIR(2));
        refresh();

        // lectura de teclado
        tecla = getch();

        // Navegación con las flechas
        if (tecla == KEY_UP && selecionado > 0) {
            selecionado--;
        }
        else if (tecla == KEY_DOWN && selecionado < 4) {
            selecionado++;
        }
        else if (tecla == 10) { // Si presiona ENTER
            if (selecionado == 0) { // Opción "Ver catálogo de juegos"
                mostrar_catalogo();
            }
            else if (selecionado == 1) { // Opción "Ver carrito de compras"
                // Aquí iría la función para mostrar el carrito
            }
            else if (selecionado == 2) { // Opción "Historial de compras"
                // Aquí iría la función para mostrar el historial
            }
            else if (selecionado == 3) { // Opción "Agregar saldo a la cuenta"
                // Aquí iría la función para agregar saldo
            }
            if (selecionado == 4) { // Opción "Cerrar sesión"
                ejecutando = 0; // Rompe el bucle de la tienda
            }
            // Las demás opciones (0, 1, 2, 3) las iremos programando poco a poco
        }

    } // <--- Esta llave cierra el bucle "while (ejecutando)"
} // <--- Esta llave cierra la función "void abrir_menu_tienda"


void mostrar_catalogo() {
    int max_y, max_x;
    int tecla;
    int ejecutando = 1;
    int seleccionado_juego = 0; 

    // Creamos un arreglo grande para almacenar los juegos que leamos del archivo
    struct Videojuego lista_juegos[100];
    int total_juegos = 0;

    // --- LEER EL ARCHIVO DE TEXTO ---
    FILE *archivo = fopen("juegos.txt", "r");
    if (archivo == NULL) {
        // Si el archivo no existe, mostramos un mensaje de error rápido
        clear();
        mvprintw(2, 5, "Error: No se pudo abrir 'juegos.txt'");
        mvprintw(4, 5, "Presiona cualquier tecla para volver...");
        getch();
        return;
    }

    // Leemos línea por línea usando fscanf hasta llegar al final del archivo (EOF)
    // El formato %[^,] significa: "lee todo el texto hasta que encuentres una coma"
    while (fscanf(archivo, "%d,%[^,],%[^,],%f,%d\n", 
                  &lista_juegos[total_juegos].id, 
                  lista_juegos[total_juegos].titulo, 
                  lista_juegos[total_juegos].plataforma, 
                  &lista_juegos[total_juegos].precio, 
                  &lista_juegos[total_juegos].stock) != EOF) {
        total_juegos++;
        if (total_juegos >= 100) break; // Límite de seguridad
    }
    fclose(archivo); // Cerramos el archivo después de leerlo

    // Si el archivo estaba vacío, evitamos que falle el programa
    if (total_juegos == 0) {
        clear();
        mvprintw(2, 5, "El catalogo esta vacio.");
        mvprintw(4, 5, "Presiona cualquier tecla para volver...");
        getch();
        return;
    }

    while (ejecutando) {
        getmaxyx(stdscr, max_y, max_x);
        clear();

        // --- ENCABEZADOS DE LA TABLA ---
        attron(A_BOLD);
        mvprintw(2, 5, "ID   %-25s %-12s %-10s %s", "GAME TITLE", "PLATFORM", "PRICE", "STOCK");
        attroff(A_BOLD);
        
        mvhline(3, 5, '=', max_x - 10);

        // --- RENDERIZADO DINÁMICO DE LOS JUEGOS ---
        // Ahora el ciclo corre hasta 'total_juegos' en lugar de un número fijo
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

        // --- BARRA INFERIOR DE GUÍA ---
        attron(COLOR_PAIR(2));
        mvhline(max_y - 1, 0, ' ', max_x);
        char *leyenda = "[ENTER] View More | [UP/DOWN] Select | [ESC] Back";
        mvprintw(max_y - 1, (max_x - strlen(leyenda)) / 2, "%s", leyenda);
        attroff(COLOR_PAIR(2));

        refresh();
        tecla = getch();

        // --- CONTROL DE NAVEGACIÓN ---
        if (tecla == KEY_UP && seleccionado_juego > 0) {
            seleccionado_juego--;
        }
        // El límite inferior ahora se adapta dinámicamente al total de juegos cargados
        else if (tecla == KEY_DOWN && seleccionado_juego < total_juegos - 1) {
            seleccionado_juego++;
        }
        else if (tecla == 27) { // ESC para volver
            ejecutando = 0;
        }
    }
}