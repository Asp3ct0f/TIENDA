#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "Menu_tienda.h"

// =========================================================
// FUNCIÓN PARA GENERAR ID Y GUARDAR EN TXT
// =========================================================
void guardar_en_txt(const char *usuario, const char *correo, const char *contrasena) {
    // 1. Intentamos abrir el archivo en modo lectura para calcular el siguiente ID
    FILE *archivo_lectura = fopen("usuarios.txt", "r");
    int id_actual = 1;

    if (archivo_lectura != NULL) {
        char linea[256];
        // Buscamos cuántas líneas contienen "ID: " para determinar el número consecutivo
        while (fgets(linea, sizeof(linea), archivo_lectura)) {
            if (strncmp(linea, "ID:", 3) == 0) {
                id_actual++;
            }
        }
        fclose(archivo_lectura);
    }

    // 2. Abrimos el archivo en modo append ("a") para añadir los datos al final
    FILE *archivo_escritura = fopen("usuarios.txt", "a");
    if (archivo_escritura == NULL) {
        return; // Si hay un error con el archivo, salimos de forma segura
    }

    // 3. Escribimos con el formato exacto que necesitas
    fprintf(archivo_escritura, "ID: %d\n", id_actual);
    fprintf(archivo_escritura, "Usuario: %s\n", usuario);
    fprintf(archivo_escritura, "Contrasena: %s\n", contrasena); // Se guarda pass_buf en el campo Hash
    fprintf(archivo_escritura, "Correo: %s\n", correo);
    fprintf(archivo_escritura, "----------------------------------------\n"); 

    fclose(archivo_escritura);
}




int main() {
    int estado = 0; // 0: Menú principal, 1: Pantalla de registro 2: Pantalla de inicio de sesión
    static char user_buf[20] = {0};
    static char mail_buf[50] = {0};
    static char pass_buf[50] = {0};

    initscr(); // iniciar ncurses
    raw(); 
    keypad(stdscr, TRUE); // Teclado 
    noecho(); // no mastrar teclado
    curs_set(0);
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK); 
    init_pair(2, COLOR_BLACK, COLOR_GREEN);//iluminacion del menu

    int max_y, max_x;
    int selecionado = 0;
    int tecla;

    char *opciones[3] = {
    "[REG]  REGISTRARSE           ",
    "[LOG]  INICIAR SESION        ",
    "[EXIT] SALIR DEL SISTEMA     "
    };

int estado_anterior = -1; // Para detectar cambios de estado

  while (1)
    {
        getmaxyx(stdscr, max_y, max_x);

        // Solo limpiamos si el estado cambió o si estamos en el menú principal
        if(estado != estado_anterior || estado == 0) {
            clear();
            estado_anterior = estado;
        }
        clear();
        bkgd(COLOR_PAIR(1));

        attron(COLOR_PAIR(2));
        mvhline(max_y - 1, 0, ' ', max_x);
        char *leyenda = "ENTER: Seleccionar | FLECHAS: Navegar | ESC: Salir";
        mvprintw(max_y - 1, (max_x - 48) / 2, "%s", leyenda);
        attroff(COLOR_PAIR(2));

        refresh();

        // --- ESTADO 0: MENÚ PRINCIPAL ---
        if (estado == 0) {
            mvprintw(2, (max_x - 43) / 2, "  ____    _    __  __ _____ ____  _   _ __  __ ");
            mvprintw(3, (max_x - 43) / 2, " / ___|  / \\  |  \\/  | ____| __ )| | | |\\ \\/ / ");
            mvprintw(4, (max_x - 43) / 2, "| |  _  / _ \\ | |\\/| |  _| |  _ \\| | | | \\  /  ");
            mvprintw(5, (max_x - 43) / 2, "| |_| |/ ___ \\| |  | | |___| |_) | |_| | / / ");
            mvprintw(6, (max_x - 43) / 2, " \\____/_/   \\_\\_|  |_|_____|____/ \\___/ /_/ ");
            
            mvprintw(8, 4, "<");
            mvhline(8, 5, '-', max_x - 10);
            mvprintw(8, max_x - 5, ">");

            int menu_x = (max_x - 35) / 2;
            mvprintw(12, menu_x, "+---------------------------------+");
            for (int i = 0; i < 3; i++) {
                mvprintw(13 + i, menu_x, "|");
                mvprintw(13 + i, menu_x + 34, "|");
                if (i == selecionado) {
                    attron(COLOR_PAIR(2));
                    mvprintw(13 + i, menu_x + 2, "%s", opciones[i]);
                    attroff(COLOR_PAIR(2));
                } else {
                    mvprintw(13 + i, menu_x + 2, "%s", opciones[i]);
                }
            }
            mvprintw(16, menu_x, "+---------------------------------+");
        }
        
  
     // ESTADO 1: PANTALLA DE REGISTRO 
        else if (estado == 1) {
            // Limpieza visual inicial de los buffers para nuevos registros
            memset(user_buf, 0, sizeof(user_buf));
            memset(mail_buf, 0, sizeof(mail_buf));
            memset(pass_buf, 0, sizeof(pass_buf));

            mvprintw(5, (max_x - 20) / 2, "--- REGISTRO ---");
            mvprintw(8, 10, "Usuario: ");
            mvprintw(9, 10, "Correo: ");
            mvprintw(10, 10, "Contraseña: ");

            // Dibujamos el botón inicialmente en un formato normal (desactivado)
            char *texto_boton = "[ Guardar Datos ]";
            int boton_x = (max_x - strlen(texto_boton)) / 2;
            int boton_y = 13;
            mvprintw(boton_y, boton_x, "%s", texto_boton);

            // --- CONFIGURACIÓN DE ENTRADA ---
            noecho();     // Controlamos la impresión manualmente
            curs_set(1);  // Mostrar cursor

            int i, ch;

            // 1. CAPTURA DE USUARIO
      
            move(8, 19);  
            i = 0;
            while (i < 19) {
                ch = getch();
                if (ch == 27) { estado = 0; break; } // ESC: Cancelar
                else if (ch == 10 || ch == KEY_ENTER) { 
                    if (i == 0) continue; // Evita dejar el campo vacío
                    user_buf[i] = '\0'; 
                    break; 
                }
                else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                    if (i > 0) { i--; printw("\b \b"); }
                }
                else { user_buf[i] = ch; printw("%c", ch); i++; }
            }

            if (estado == 0) { noecho(); curs_set(0); continue; }

            
            // 2. CAPTURA DE CORREO
  
            move(9, 18);  
            i = 0;
            while (i < 49) {
                ch = getch();
                if (ch == 27) { estado = 0; break; } // ESC: Cancelar
                else if (ch == 10 || ch == KEY_ENTER) { 
                    if (i == 0) continue; // Evita dejar el campo vacío
                    mail_buf[i] = '\0'; 
                    break; 
                }
                else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                    if (i > 0) { i--; printw("\b \b"); }
                }
                else { mail_buf[i] = ch; printw("%c", ch); i++; }
            }

            if (estado == 0) { noecho(); curs_set(0); continue; }

        
            // 3. CAPTURA DE CONTRASEÑA (OCULTA CON '*')

            move(10, 22);  
            i = 0;
            while (i < 49) {
                ch = getch();
                if (ch == 27) { estado = 0; break; } // ESC: Cancelar
                else if (ch == 10 || ch == KEY_ENTER) { 
                    if (i == 0) continue; // Evita dejar el campo vacío
                    pass_buf[i] = '\0'; 
                    break; 
                }
                else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                    if (i > 0) { i--; printw("\b \b"); }
                }
                else { 
                    pass_buf[i] = ch; 
                    printw("*"); // Imprime un asterisco en lugar del caracter real
                    i++; 
                }
            }

            if (estado == 0) { noecho(); curs_set(0); continue; }

            
            // 4. ENFOQUE INTERACTIVO DEL BOTÓN "GUARDAR"
          
            curs_set(0); // Ocultamos el cursor físico

            // Iluminamos el botón (Efecto Hover/Selected)
            attron(A_REVERSE);
            mvprintw(boton_y, boton_x, "%s", texto_boton);
            attroff(A_REVERSE);
            refresh();

            // Espera de confirmación del botón
            while (1) {
                ch = getch();
                if (ch == 27) { // ESC Cancela
                    estado = 0; 
                    break; 
                } 
                else if (ch == 10 || ch == KEY_ENTER) { // ENTER guarda
                    // Quitamos el carácter raro para evitar que se congele la terminal
                    mvprintw(15, (max_x - 40) / 2, "[ OK ] Datos guardados con exito. Volviendo...");
                    refresh();
                    
                    // --- AQUÍ VA TU LÓGICA DE INTERPROCESO / SERVIDOR ---
                    // enviar_datos_servidor(user_buf, mail_buf, pass_buf);
                    
                    guardar_en_txt(user_buf, mail_buf, pass_buf);
                    
                    sleep(2);
                    estado = 0;
                    break;
                
                    
                    sleep(2);   // Pausa visual de 2 segundos
                    estado = 0; // Cambiamos el estado para regresar al menú
                    break;      // Rompemos el ciclo del botón
                }
            }
            
            noecho();
            curs_set(0);
            flushinp();
            continue; // Regresa al inicio del while(1) principal
        }
        
        
       // --- ESTADO 2: PANTALLA DE INICIAR SESIÓN (LOGIN) ---
        else if (estado == 2) {
            // Buffers locales o estáticos para capturar las credenciales de intento de login
            static char login_user[20];
            static char login_pass[50];

            // Limpieza visual de buffers al entrar a la pantalla
            memset(login_user, 0, sizeof(login_user));
            memset(login_pass, 0, sizeof(login_pass));

            mvprintw(5, (max_x - 24) / 2, "--- INICIAR SESION ---");
            mvprintw(8, 10, "Usuario: ");
            mvprintw(9, 10, "Contraseña: ");

            // Dibujamos el botón inicialmente desmarcado
            char *texto_boton = "[ Iniciar Sesion ]";
            int boton_x = (max_x - strlen(texto_boton)) / 2;
            int boton_y = 12; // Un renglón más arriba que el registro ya que son menos campos
            mvprintw(boton_y, boton_x, "%s", texto_boton);

            // Configuramos la entrada interactiva
            noecho();     
            curs_set(1);  

            int i, ch;

            // =========================================================
            // 1. CAPTURA DE USUARIO
            // =========================================================
            move(8, 19);  
            i = 0;
            while (i < 19) {
                ch = getch();
                if (ch == 27) { estado = 0; break; } // ESC: Regresar al menú
                else if (ch == 10 || ch == KEY_ENTER) { 
                    if (i == 0) continue; // Evita avanzar si está vacío
                    login_user[i] = '\0'; 
                    break; 
                }
                else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                    if (i > 0) { i--; printw("\b \b"); }
                }
                else { login_user[i] = ch; printw("%c", ch); i++; }
            }

            if (estado == 0) { noecho(); curs_set(0); continue; }

            // =========================================================
            // 2. CAPTURA DE CONTRASEÑA (OCULTA CON '*')
            // =========================================================
            move(9, 22);  
            i = 0;
            while (i < 49) {
                ch = getch();
                if (ch == 27) { estado = 0; break; } // ESC: Regresar al menú
                else if (ch == 10 || ch == KEY_ENTER) { 
                    if (i == 0) continue; // Evita avanzar si está vacío
                    login_pass[i] = '\0'; 
                    break; 
                }
                else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                    if (i > 0) { i--; printw("\b \b"); }
                }
                else { login_pass[i] = ch; printw("*"); i++; }
            }

            if (estado == 0) { noecho(); curs_set(0); continue; }

            // =========================================================
            // 3. ENFOQUE INTERACTIVO DEL BOTÓN "INICIAR SESIÓN"
            // =========================================================
            curs_set(0); // Ocultamos el cursor físico para resaltar el botón

            // Iluminamos el botón invirtiendo colores
            attron(A_REVERSE);
            mvprintw(boton_y, boton_x, "%s", texto_boton);
            attroff(A_REVERSE);
            refresh();

            // Espera de interacción sobre el botón
            while (1) {
                ch = getch();
                if (ch == 27) { 
                    estado = 0; // ESC: Aborta y regresa al menú
                    break; 
                } 
                else if (ch == 10 || ch == KEY_ENTER) {
                    mvprintw(15, (max_x - 40) / 2, "[ OK ] Validando credenciales...");
                    refresh();
                    sleep(1); // Simulación de proceso de validación

                    abrir_menu_tienda(login_user); // Si el login es exitoso, abrimos el menú de la tienda con el usuario autenticado
                    
                    // --- AQUÍ COMPARARÁS CON EL TXT O ENVIARÁS AL SERVIDOR ---
                    // En la siguiente fase leeremos el txt para validar si login_user y login_pass coinciden.
                    
                  ;
                    estado = 0; // Regresa al menú principal por ahora
                    break;
                }
            }
            
            noecho();
            curs_set(0);
            flushinp();
            continue;
        }
        

        // Lógica de entrada
        tecla = getch();
        if (estado == 0) {
            if (tecla == KEY_UP && selecionado > 0) selecionado--;
            else if (tecla == KEY_DOWN && selecionado < 2) selecionado++;
            else if (tecla == 10) { // Si presionas ENTER
                if (selecionado == 0) estado = 1; // Ir a registro
                else if (selecionado == 1) estado = 2;
                else if (selecionado == 2) break; // Salir
            }
        }
        
        if (tecla == 27) {
            if (estado == 0) break; // Salir del programa
            else estado = 0;        // Volver al menú
        }
    }

    endwin(); // 
    return 0;
}