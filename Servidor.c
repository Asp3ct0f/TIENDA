#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //librerias para los semaforos y memoria compartida
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>
#include <string.h>
#include <signal.h> // NUEVA LIBRERIA PARA CAPTURAR EL CTRL+C
#include <time.h> // libreria para obtener fecha y hora
#include "cifrado.h"
#define PERMISOS 0644
#define LOGIN 1
#define  REGISTRO 2
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

char *acciones[] = {
    "INVALIDO",
    "LOGIN",
    "REGISTRO",
    "VER_PRODUCTOS",
    "AGREGAR_CARRITO",
    "VER_CARRITO",
    "COMPRAR_CARRITO",
    "VER_HISTORIAL",
    "BUSCAR_PRODUCTO",
    "VER_USUARIOS",
    "CREAR_USUARIO_ADMIN",
    "MODIFICAR_USUARIO",
    "ELIMINAR_USUARIO",
    "REPORTE_DIARIO",
    "REPORTE_SEMANAL",
    "REPORTE_MENSUAL",
    "AGREGAR_PRODUCTO_ADMIN",
    "MODIFICAR_STOCK",
    "ELIMINAR_PRODUCTO",
    "MODIFICAR_MI_CUENTA"
};


struct Productos //estructura para los datos de los productos
{
    int id; //id del producto
    char nombre[50]; //nombre del producto
    char plataforma[50]; //nombre de la plataforma que hizo el juego
    float precio; //precio del producto
    int stock; //inventario 

};

struct Datos //estructura para recabar los datos del usuario
{
    int accion; //lo que decida hacer el usuario
    int es_admin; // Variable para identificar si el usuario es administrador

    char usuario[50]; //nombre del usuario
    char password[100]; //contrasena del usuario

    char correo[70]; //correo del usuario
    char respuesta[50]; //mensaje de respuesta
    char mensaje[200]; // Variable auxiliar para mensajes y busquedas

    char productos[1500]; //variable de productos para la memoria compartida

    // Variable para manejar productos
    struct Productos producto;
};


struct Datos *memoria; //la variable de memoria es un bloque estructurado

// VARIABLES GLOBALES PARA MANEJAR AMBOS SEMAFOROS Y LA MEMORIA
int semaforo_servidor;
int semaforo_cliente;
int shmid; // Pasamos shmid a global para poder liberarlo al cerrar

// PROTOTIPO DE LA FUNCION PARA BUSCAR PRODUCTOS
void BuscarProducto(struct Datos *memoria);

// Funcion para mostrar usuarios registrados
void VerUsuarios(struct Datos *memoria);

//Hilo para el cliente
void *Hilo(void *argumentos) //funcion del hilo (lo que va a hacer)
{
    // === RESPALDO LOCAL DE LOS DATOS INMEDIATAMENTE PARA EVITAR CONDICIONES DE CARRERA ===
    int accion_local = memoria->accion;
    char usuario_local[50];
    char password_local[100];
    char correo_local[70];
    
    strcpy(usuario_local, memoria->usuario);
    strcpy(password_local, memoria->password);
    strcpy(correo_local, memoria->correo);

    printf("\nCliente conectado\n"); //mensaje para ver la conexion

    printf("ACCION RECIBIDA: %d\n", accion_local);

    if(accion_local >= LOGIN && accion_local <= MODIFICAR_MI_CUENTA)
    {
        printf("Operacion recibida: %s\n", acciones[accion_local]); //mostrar la accion 
    }

    else
    {
        printf("Operacion desconocida\n"); //por si eligen otra fuera del rango
    }
    printf("Usuario: %s\n", usuario_local);  //se muestra el nombre del usuario
    printf("Password: %s\n", password_local); // se muestra la contrasena
    printf("Correo: %s\n", correo_local);

    //REGISTRO EN EL ARCHIVO

    if(accion_local == REGISTRO){ //si se decide hacer un registro

        FILE *archivo; //usamos un puntero del archivo

        //EVITAR USUARIOS REPETIDOS
        FILE *archivo_buscador; //puntero para buscar en el archivo un repetido
        archivo_buscador=fopen("usuarios.txt", "r"); //abrir el archivo en forma de read "r"
        int usuario_existe = 0; // banderita
    
        if (archivo_buscador != NULL) { //si encuentra el archivo
            char linea[200]; //variable de linea del archivo
            while (fgets(linea, sizeof(linea), archivo_buscador)) {  //va leyendo hasta el final del archivo
                if (strncmp(linea, "Usuario: ", 9) == 0) { //compara las cadenas, el 9 es por los caracteres de "Usuario: ", si son iguales las cadenas devuelve un 0
                    char usuario_en_archivo[50];
                    sscanf(linea + 9, "%49[^\n]", usuario_en_archivo);// pedimos que lea ignorando los 9 caracteres antes mencionados, lee los 49 caracteres hasta encontrar un salto
                    
                    // Limpieza de posibles retornos de carro o saltos de linea invisibles de Windows/Linux
                    usuario_en_archivo[strcspn(usuario_en_archivo, "\r\n")] = 0;
                    
                    if (strcmp(usuario_en_archivo, usuario_local) == 0) { //si son iguales
                        usuario_existe = 1; //ponemos en 1 la variable
                        break;
                    }
                }
            }
            fclose(archivo_buscador);
        }

        int correo_valido=0;
        int password_valida=0;

        //PARA VALIDAR CORREO
        if(strchr(correo_local, '@') !=NULL){ //si encutra una @
            correo_valido=1; //modificamos el valor de la variable
        }


        //PARA VALIDAR CONTRASENA
        if(strlen(password_local) >= 8){ //si la longitud de la memoria es de 8 caracteres o mas, es valida
            password_valida=1; //modificamos el valor de la variable
        }

        //VALIDACIONES

        if(!correo_valido){ //si no es valido
            strcpy(memoria->respuesta, "ERROR. Correo no valido"); //copiamos la cadena y se la mandamos al cliente
            printf("Correo no valido\n");
        }

        else if(!password_valida){ //si no es valida
            strcpy(memoria->respuesta, "ERROR. Password no valida (Debe tener al menos 8 caracteres)");//copiamos la cadena y se la mandamos al cliente
            printf("Password invalida \n");
        }

        else if (usuario_existe)
        {
            strcpy(memoria->respuesta, "ERROR. Usuario ya existente. Elija otro nombre de usuario."); //copiamos la cadena y se la mandamos al cliente
            printf("Intento de registro fallido: El usuario %s ya existe.\n", usuario_local);        
        }

        else{

            archivo = fopen("usuarios.txt", "a"); //abrimos el archivo de usuarios con la funcion de append 

            if(archivo != NULL){ // si el puntero es valido (si "encuentra el archivo")

                //Escribir los datos del usuario dentro de el archivo
                fprintf(archivo,"Usuario: %s\n", usuario_local); //escribe el usuario
                fprintf(archivo,"Password: %s\n", password_local); //escribe la contrasena
                fprintf(archivo,"Correo: %s\n", correo_local);//escribe el correo del usuario
                fprintf(archivo,"------------------------------------\n"); //linea divisora (para mejor orden)

                fclose(archivo); //cierra el archivo

                strcpy(memoria->respuesta, "Registro exitoso"); //copia la cadena "Registro exitoso" y la pone en el resultado
                printf("Usuario registrado \n");
            }
            else{
                strcpy(memoria->respuesta, "Error al abrir archivo"); //si ocurre un error en el archivo
                printf("Error al abrir archivo\n");
            }
        }
    }

    else if (accion_local == LOGIN) 
    {
        FILE *archivo; // usamos un puntero del archivo

        char linea[200];
        char usuario_archivo[50];
        char password_archivo[100];
        int usuario_correcto = 0; // banderita para indicar si las credenciales son correctas

        archivo = fopen("usuarios.txt", "r"); // abrimos el archivo de los usuarios en modo lectura ("r" read)

        if (archivo != NULL) 
        { // si "encuentra" el archivo
            while (fgets(linea, sizeof(linea), archivo)) 
            {
                if (strncmp(linea, "Usuario: ", 9) == 0) 
                { // leemos la linea del usuario

                    sscanf(linea + 9, "%49[^\n]", usuario_archivo); // Extraemos ignorando el prefijo
                    usuario_archivo[strcspn(usuario_archivo, "\r\n")] = 0; // Limpieza crítica de saltos de línea

                    if (fgets(linea, sizeof(linea), archivo) && strncmp(linea, "Password: ", 10) == 0) 
                    {
                        sscanf(linea + 10, "%99[^\n]", password_archivo);  // Leemos la línea siguiente que debe ser el password
                        password_archivo[strcspn(password_archivo, "\r\n")] = 0; // Limpieza crítica de saltos de línea

                        // Verificamos credenciales
                        if (strcmp(usuario_archivo, usuario_local) == 0 && strcmp(password_archivo, password_local) == 0) 
                        {
                            usuario_correcto = 1;
                            break;
                        }
                    }
                }
            }

            fclose(archivo);

            // Respuesta final exacta y limpia sin caracteres extrañas
            if (usuario_correcto) 
            {
                strcpy(memoria->respuesta, "Login exitoso");

		// Verificamos si el usuario que inicio sesion es administrador
		if(strcmp(usuario_local, "admin") == 0)
		{
		    // Marcamos al usuario como administrador
		    memoria->es_admin = 1;
		}
		else
		{
		    // Usuario normal
		    memoria->es_admin = 0;
		}
                printf("Login correcto para: %s\n", usuario_local);
            } 
            else 
            {
                strcpy(memoria->respuesta, "Usuario o password incorrectos");
                printf("Login fallido para: %s\n", usuario_local);
            }
        } 
        else 
        {
            strcpy(memoria->respuesta, "Error al abrir usuarios");
            printf("Error al abrir archivo\n");
        }
    }

    else if (accion_local == VER_PRODUCTOS)  //para leer el catalogo
    {
        FILE *archivo_productos; //apuntador para el archivo
        archivo_productos=fopen("productos.txt", "r"); //abrimos productos.txt en forma de lectura (read)
        if (archivo_productos != NULL) 
        {
            char linea[250];
            strcpy(memoria->productos, ""); // Limpiamos la memoria

            while (fgets(linea, sizeof(linea), archivo_productos))   // leemos el archivo línea por línea y lo metemos en la memoria compartida
            {
                strcat(memoria->productos, linea); //concatenamos las cadenas
            }
            fclose(archivo_productos); //cerramos el archivo
            strcpy(memoria->respuesta, "OK"); // le decimos al cliente que todo salio bien
        } 
        else 
        {
            strcpy(memoria->respuesta, "Error al entrar al catalogo"); //informamos que hubo un error
            printf("Error: No se pudo abrir productos.txt\n"); //mensaje en el servidor
        }
    }

    else if (accion_local == AGREGAR_CARRITO) //funcion para agregar al carrito y restar stock
    {
        int id_buscado, cantidad_pedida;
        // el cliente nos pasa el ID y la cantidad separados por un punto y coma en memoria->productos
        sscanf(memoria->productos, "%d;%d", &id_buscado, &cantidad_pedida); //leemos los valores que dio el cliente

        FILE *archivo_productos = fopen("productos.txt", "r"); //abrimos el archivo de productos in read
        FILE *archivo_temporal = fopen("productos_tmp.txt", "w"); // archivo temporal para actualizar el stock

        int encontrado = 0;
        int stock_suficiente = 0;
        struct Productos p_actual; //variable del tipo Productos que indica el producto actual 
        char linea[250];

        if (archivo_productos != NULL && archivo_temporal != NULL) //si encuentra ambos archivos
        {
            while (fgets(linea, sizeof(linea), archivo_productos))  //vamos linea por linea
            {
                sscanf(linea, "%d;%[^;];%[^;];%f;%d", 
                       &p_actual.id, p_actual.nombre, p_actual.plataforma, &p_actual.precio, &p_actual.stock); //leemos la linea del producto

                if (p_actual.id == id_buscado) //si el id es el buscado
                {
                    encontrado = 1; //cambiamos valor
                    if (p_actual.stock >= cantidad_pedida) //si el stock del producto es igual o mayor que la pedida
                    {
                        stock_suficiente = 1; //cambiamos el valor
                        p_actual.stock -= cantidad_pedida; // restamos el stock

                        // === SE CORRIGE AQUÍ: Metemos el producto al carrito EN CALIENTE antes de que avance la lectura ===
                        char nombre_carrito[100];
                        sprintf(nombre_carrito, "carrito_%s.txt", usuario_local);
                        FILE *archivo_carrito = fopen(nombre_carrito, "a");

                        if (archivo_carrito != NULL) 
                        {
                            for (int i = 0; i < cantidad_pedida; i++) // guardamos en el carrito la cantidad que se llevo
                            {
                                fprintf(archivo_carrito, "%d;%s;%s;%.2f\n", 
                                        id_buscado, p_actual.nombre, p_actual.plataforma, p_actual.precio);
                            }
                            fclose(archivo_carrito); //cerramos el archivo
                        }
                    }
                }
                // Reescribimos todo el catálogo en el archivo temporal con los stocks actualizados
                fprintf(archivo_temporal, "%d;%s;%s;%.2f;%d\n", 
                        p_actual.id, p_actual.nombre, p_actual.plataforma, p_actual.precio, p_actual.stock);
            }
            fclose(archivo_productos); //cerramos archivos
            fclose(archivo_temporal);

            
            rename("productos_tmp.txt", "productos.txt"); // reemplazamos el archivo temporal por de productos

            if (!encontrado) 
            {
                strcpy(memoria->respuesta, "ERROR. Producto no encontrado.");
            }
            else if (!stock_suficiente) 
            {
                strcpy(memoria->respuesta, "ERROR. Stock insuficiente.");
            }
            else 
            {
                strcpy(memoria->respuesta, "Producto(s) agregado(s) al carrito");
                printf("Stock actualizado para ID %d. Carrito modificado para %s.\n", id_buscado, usuario_local);
            }
        } 
        else 
        {
            if (archivo_productos) fclose(archivo_productos);
            if (archivo_temporal) fclose(archivo_temporal);
            strcpy(memoria->respuesta, "Error en el servidor."); //si algo sale mal, arroja error
        }
    }

    else if (accion_local == VER_CARRITO) //funcion para ver el contenido del carrito
    {
        char nombre_archivo[100];
        sprintf(nombre_archivo, "carrito_%s.txt", usuario_local); // buscamos el archivo del usuario

        FILE *archivo_carrito; //apuntador al archivo
        archivo_carrito = fopen(nombre_archivo, "r"); // abrimos en modo lectura (r)
        if (archivo_carrito != NULL) //si no hay problemas
        {
            char linea[250];
            strcpy(memoria->productos, ""); // limpiamos la memoria antes de llenarla

            while (fgets(linea, sizeof(linea), archivo_carrito)) //vamos fila por fila 
            {
                strcat(memoria->productos, linea); // vamos concatenando cada producto del carrito
            }
            fclose(archivo_carrito); //cerramos el archivo
            strcpy(memoria->respuesta, "OK"); // todo salio bien
        } 
        else 
        {
            
            strcpy(memoria->productos, ""); 
            strcpy(memoria->respuesta, "VACIO"); //si no se encuentra el archivo es porque no hay carrito
        }
    }


    //FUNCION PARA PAGAR EL CARRITO
    else if (accion_local == COMPRAR_CARRITO) 
    {
        char archivo_car[100];
        char archivo_hist[100];
        sprintf(archivo_car, "carrito_%s.txt", usuario_local); //buscamos el carrito del usuario
        sprintf(archivo_hist, "historial_%s.txt", usuario_local); //buscamos el historial del usuario

        FILE *f_car = fopen(archivo_car, "r"); //abrimos el archivo del carrito en formato read
        if (f_car != NULL) 
        {
            FILE *f_hist = fopen(archivo_hist, "a"); // abrimos historial en modo append
            
	    // Archivo global de ventas
	    FILE *f_global = fopen("ventas_globales.txt", "a");
	    if (f_hist != NULL && f_global != NULL)
            {
                char linea[250];
                // VARIABLES PARA OBTENER FECHA ACTUAL
		time_t tiempo_actual;

		struct tm *info_tiempo;

		char fecha[50];


		// OBTENEMOS FECHA ACTUAL
		time(&tiempo_actual);

		info_tiempo = localtime(&tiempo_actual);


		// FORMATO DE FECHA
		strftime(fecha, sizeof(fecha), "%Y-%m-%d", info_tiempo);
		fprintf(f_hist, "Fecha: %s\n\n", fecha);
		// Guardamos fecha en ventas globales
		fprintf(f_global, "Fecha: %s\n\n", fecha);


		// Guardamos usuario
		fprintf(f_hist, "Usuario: %s\n\n", usuario_local);
		// Guardamos usuario en ventas globales
		fprintf(f_global, "Usuario: %s\n\n", usuario_local);

		// TITULO DEL DETALLE
		fprintf(f_hist, "=== DETALLE DE COMPRA ===\n");
		// Titulo para ventas globales
		fprintf(f_global, "=== DETALLE DE COMPRA ===\n");
                while (fgets(linea, sizeof(linea), f_car))  //vamos linea por linea
                {
                    fprintf(f_hist, "%s", linea); // copiamos cada producto al historial
			// Guardamos tambien en historial global
			fprintf(f_global, "%s", linea);
                }
                fprintf(f_hist, "------------------------------------\n");
		// Separador en historial global
		fprintf(f_global, "------------------------------------\n");
                fclose(f_hist); //cerramos historial
		// Cerramos historial global
		fclose(f_global);
                fclose(f_car); //cerramos carrito

                remove(archivo_car); // borramos el archivo del carrito (simular que se pago)
                strcpy(memoria->respuesta, "Compra procesada con exito. Muchas gracias"); //informamos al cliente del pago
            } 
            else 
            {
                fclose(f_car); //cerrramos el carrito
                strcpy(memoria->respuesta, "Error al escribir el historial."); // informamos que no se pudo obtener el historial
            }
        } 
        else 
        {
            strcpy(memoria->respuesta, "El carrito esta vacio.\n");
        }
    }

	else if (accion_local == BUSCAR_PRODUCTO)
	{
    		BuscarProducto(memoria);
	}

    //FUNCION PARA VER EL HISTORIAL 
    else if (accion_local == VER_HISTORIAL) 
    {
        char nombre_archivo[100];
        sprintf(nombre_archivo, "historial_%s.txt", usuario_local); //buscamos el arcchivo historial del usuario

        FILE *archivo_hist = fopen(nombre_archivo, "r"); //apuntador al archivo y lo usamos nomas para leer
        if (archivo_hist != NULL) 
        {
            char linea[250];
            strcpy(memoria->productos, ""); 
            while (fgets(linea, sizeof(linea), archivo_hist))  //vamos linea por linea
            {
                strcat(memoria->productos, linea); //concatenamos 
            }
            fclose(archivo_hist); //cerramos el archivo de historial
            strcpy(memoria->respuesta, "OK"); //mansdamos un OK si todo esta bien
        } 
        else 
        {
            strcpy(memoria->productos, ""); //limpiamos
            strcpy(memoria->respuesta, "VACIO"); //si no hay nada
        }
    }

        // ACCION PARA VER USUARIOS
        else if(accion_local == VER_USUARIOS)
        {

            // Llamamos funcion para mostrar usuarios
            VerUsuarios(memoria);
        }

	// FUNCION PARA CREAR USUARIO DESDE ADMIN
	else if(accion_local == CREAR_USUARIO_ADMIN)
	{

	    FILE *archivo;

	    char password_cifrada[100];


	    // Ciframos password
	    contrasenacif(password_local, password_cifrada);


	    // Abrimos archivo en modo agregar
	    archivo = fopen("usuarios.txt", "a");


	    // Verificamos si se pudo abrir
	    if(archivo != NULL)
	    {

	        // Escribimos los datos del usuario dentro del archivo
		fprintf(archivo,"Usuario: %s\n", usuario_local);

		fprintf(archivo,"Password: %s\n", password_cifrada);

		fprintf(archivo,"Correo: %s\n", correo_local);

		fprintf(archivo,"------------------------------------\n");

	        fclose(archivo);


	        // Mandamos respuesta exitosa
	        strcpy(memoria->respuesta, "Usuario creado correctamente");
	    }
	    else
	    {

	        // Error al abrir archivo
	        strcpy(memoria->respuesta, "Error al crear usuario");
	    }
	}
    
	// FUNCION PARA MODIFICAR USUARIO
	else if(accion_local == MODIFICAR_USUARIO)
	{

	    FILE *archivo;
	    FILE *temp;


	    char linea[300];

	    char usuario_archivo[100];

	    char password_archivo[100];

	    char correo_archivo[100];


	    int encontrado = 0;


	    char password_cifrada[100];


	    // Ciframos nueva password
	    contrasenacif(password_local, password_cifrada);


	    // Abrimos archivo original
	    archivo = fopen("usuarios.txt", "r");


	    // Creamos archivo temporal
	    temp = fopen("temp.txt", "w");
	

	    // Verificamos archivos
	    if(archivo == NULL || temp == NULL)
	    {
	        strcpy(memoria->respuesta, "Error al abrir archivos");
	   }
	    else
	    {

	        // Recorremos archivo linea por linea
	        while(fgets(linea, sizeof(linea), archivo))
	        {

	            // Verificamos linea usuario
	            if(strstr(linea, "Usuario: ") != NULL)
	            {

	                sscanf(linea, "Usuario: %[^\n]", usuario_archivo);


	                // Leemos password
	                fgets(linea, sizeof(linea), archivo);
	                sscanf(linea, "Password: %[^\n]", password_archivo);


	                // Leemos correo
	                fgets(linea, sizeof(linea), archivo);
	                sscanf(linea, "Correo: %[^\n]", correo_archivo);


	                // Leemos separador
	                fgets(linea, sizeof(linea), archivo);


	                // Verificamos usuario a modificar
	                if(strcmp(usuario_archivo, memoria->mensaje) == 0)
	                {

	                    encontrado = 1;


	                    // Escribimos nuevos datos
	                    fprintf(temp,"Usuario: %s\n", usuario_local);

	                    fprintf(temp,"Password: %s\n", password_cifrada);

	                    fprintf(temp,"Correo: %s\n", correo_local);

	                    fprintf(temp,"------------------------------------\n");
	                }
	                else
	                {

	                    // Reescribimos usuario original
	                    fprintf(temp,"Usuario: %s\n", usuario_archivo);

	                    fprintf(temp,"Password: %s\n", password_archivo);

	                    fprintf(temp,"Correo: %s\n", correo_archivo);

	                    fprintf(temp,"------------------------------------\n");
        	        }
	            }
	        }


	        fclose(archivo);

	        fclose(temp);


	        // Eliminamos archivo viejo
	        remove("usuarios.txt");


	        // Renombramos temporal
        	rename("temp.txt", "usuarios.txt");


	        // Verificamos resultado
	        if(encontrado)
	        {
	            strcpy(memoria->respuesta, "Usuario modificado correctamente");
	        }
	        else
	        {
	            strcpy(memoria->respuesta, "Usuario no encontrado");
	        }
	    }
	}
	// FUNCION PARA ELIMINAR USUARIO
else if(accion_local == ELIMINAR_USUARIO)
{

    FILE *archivo;

    FILE *temp;


    char linea[300];

    char usuario_archivo[100];

    char password_archivo[100];

    char correo_archivo[100];


    int encontrado = 0;


    // Abrimos archivo original
    archivo = fopen("usuarios.txt", "r");


    // Creamos archivo temporal
    temp = fopen("temp.txt", "w");


    // Verificamos archivos
    if(archivo == NULL || temp == NULL)
    {
        strcpy(memoria->respuesta, "Error al abrir archivos");
    }
    else
    {

        // Recorremos archivo linea por linea
        while(fgets(linea, sizeof(linea), archivo))
        {

            // Verificamos linea de usuario
            if(strstr(linea, "Usuario: ") != NULL)
            {

                // Leemos usuario
                sscanf(linea, "Usuario: %[^\n]", usuario_archivo);


                // Leemos password
                fgets(linea, sizeof(linea), archivo);
                sscanf(linea, "Password: %[^\n]", password_archivo);


                // Leemos correo
                fgets(linea, sizeof(linea), archivo);
                sscanf(linea, "Correo: %[^\n]", correo_archivo);


                // Leemos separador
                fgets(linea, sizeof(linea), archivo);


                // Verificamos si es el usuario a eliminar
                if(strcmp(usuario_archivo, memoria->mensaje) == 0)
                {

                    // Marcamos encontrado
                    encontrado = 1;
                }
                else
                {

                    // Reescribimos usuario que NO se elimina
                    fprintf(temp,"Usuario: %s\n", usuario_archivo);

                    fprintf(temp,"Password: %s\n", password_archivo);

                    fprintf(temp,"Correo: %s\n", correo_archivo);

                    fprintf(temp,"------------------------------------\n");
                }
            }
        }


        fclose(archivo);

        fclose(temp);


        // Verificamos si el usuario fue encontrado
        if(encontrado)
        {

            // Eliminamos archivo viejo
            remove("usuarios.txt");


            // Renombramos temporal
            rename("temp.txt", "usuarios.txt");


            strcpy(memoria->respuesta, "Usuario eliminado correctamente");
        }
        else
        {

            // Eliminamos temporal
            remove("temp.txt");


            strcpy(memoria->respuesta, "Usuario no encontrado");
        }
    }
}

// FUNCION PARA GENERAR REPORTE DIARIO
else if(accion_local == REPORTE_DIARIO)
{

    FILE *historial;

    FILE *reporte;


    char linea[300];

    float total_ventas = 0;

    float precio;


    char fecha_actual[50];

    time_t tiempo_actual;

    struct tm *info_tiempo;


    // OBTENEMOS FECHA ACTUAL
    time(&tiempo_actual);

    info_tiempo = localtime(&tiempo_actual);

    strftime(fecha_actual, sizeof(fecha_actual), "%Y-%m-%d", info_tiempo);

	// Abrimos historial global de ventas
	historial = fopen("ventas_globales.txt", "r");


    // Verificamos historial
    if(historial == NULL)
    {

        strcpy(memoria->respuesta, "No existe historial para generar reporte");
    }
    else
    {

        // Creamos archivo reporte
        reporte = fopen("reporte_diario.txt", "w");


        fprintf(reporte, "===== REPORTE DIARIO =====\n\n");

        fprintf(reporte, "Fecha: %s\n\n", fecha_actual);

	// Guardamos mensaje final con el reporte
	sprintf(memoria->respuesta,
        "Reporte diario generado correctamente");

	// Limpiamos memoria de productos
	strcpy(memoria->productos, "");
        
	// Recorremos historial
        while(fgets(linea, sizeof(linea), historial))
        {

            if(strchr(linea, ';') != NULL)
{

    int id_producto;

    char nombre[150];

    char plataforma[100];


    // Obtenemos datos del producto
    sscanf(linea,
           "%d;%[^;];%[^;];%f",
           &id_producto,
           nombre,
           plataforma,
           &precio);

	// Guardamos tambien en memoria para mostrarlo en consola
	sprintf(linea,
        "Producto: %s\nPlataforma: %s\nPrecio: %.2f\n\n",
        nombre,
        plataforma,
        precio);

	strcat(memoria->productos, linea);
    // Sumamos venta
    total_ventas += precio;


    // Guardamos detalle en reporte
    fprintf(reporte,
            "Producto: %s\nPlataforma: %s\nPrecio: %.2f\n\n",
            nombre,
            plataforma,
            precio);
}
        }


        fprintf(reporte,
        "====================================\n");

	fprintf(reporte,
        "TOTAL VENDIDO: %.2f\n",
        total_ventas);

	// Guardamos total en memoria
	sprintf(linea,
        "====================================\nTOTAL VENDIDO: %.2f\n",
        total_ventas);

	strcat(memoria->productos, linea);

        fclose(historial);

        fclose(reporte);


        strcpy(memoria->respuesta,
               "Reporte diario generado correctamente");
    }
}

// FUNCION PARA GENERAR REPORTE SEMANAL
else if(accion_local == REPORTE_SEMANAL)
{

    FILE *ventas;

    FILE *reporte;


    char linea[300];

    float total_ventas = 0;

    float precio;


    int id_producto;

    char nombre[150];

    char plataforma[100];


    // Abrimos historial global
    ventas = fopen("ventas_globales.txt", "r");


    // Verificamos archivo
    if(ventas == NULL)
    {

        strcpy(memoria->respuesta,
               "No existen ventas registradas");
    }
    else
    {

        // Creamos archivo reporte
        reporte = fopen("reporte_semanal.txt", "w");


        // Limpiamos memoria compartida
        strcpy(memoria->productos, "");


        fprintf(reporte,
                "===== REPORTE SEMANAL =====\n\n");


        strcat(memoria->productos,
               "===== REPORTE SEMANAL =====\n\n");


        // Recorremos archivo
        while(fgets(linea, sizeof(linea), ventas))
        {

            // Detectamos productos
            if(strchr(linea, ';') != NULL)
            {

                sscanf(linea,
                       "%d;%[^;];%[^;];%f",
                       &id_producto,
                       nombre,
                       plataforma,
                       &precio);


                // Sumamos ventas
                total_ventas += precio;


                // Guardamos en txt
                fprintf(reporte,
                        "Producto: %s\nPlataforma: %s\nPrecio: %.2f\n\n",
                        nombre,
                        plataforma,
                        precio);


                // Guardamos en consola
                sprintf(linea,
                        "Producto: %s\nPlataforma: %s\nPrecio: %.2f\n\n",
                        nombre,
                        plataforma,
                        precio);

                strcat(memoria->productos, linea);
            }
        }


        fprintf(reporte,
                "====================================\n");

        fprintf(reporte,
                "TOTAL SEMANAL: %.2f\n",
                total_ventas);


        strcat(memoria->productos,
               "====================================\n");


        sprintf(linea,
                "TOTAL SEMANAL: %.2f\n",
                total_ventas);

        strcat(memoria->productos, linea);


        fclose(ventas);

        fclose(reporte);


        strcpy(memoria->respuesta,
               "Reporte semanal generado correctamente");
    }
}

// FUNCION PARA GENERAR REPORTE MENSUAL
else if(accion_local == REPORTE_MENSUAL)
{

    FILE *ventas;

    FILE *reporte;


    char linea[300];

    float total_ventas = 0;

    float precio;


    int id_producto;

    char nombre[150];

    char plataforma[100];


    // Abrimos historial global
    ventas = fopen("ventas_globales.txt", "r");


    // Verificamos archivo
    if(ventas == NULL)
    {

        strcpy(memoria->respuesta,
               "No existen ventas registradas");
    }
    else
    {

        // Creamos archivo reporte
        reporte = fopen("reporte_mensual.txt", "w");


        // Limpiamos memoria compartida
        strcpy(memoria->productos, "");


        fprintf(reporte,
                "===== REPORTE MENSUAL =====\n\n");


        strcat(memoria->productos,
               "===== REPORTE MENSUAL =====\n\n");


        // Recorremos archivo
        while(fgets(linea, sizeof(linea), ventas))
        {

            // Detectamos productos
            if(strchr(linea, ';') != NULL)
            {

                sscanf(linea,
                       "%d;%[^;];%[^;];%f",
                       &id_producto,
                       nombre,
                       plataforma,
                       &precio);


                // Sumamos ventas
                total_ventas += precio;


                // Guardamos en txt
                fprintf(reporte,
                        "Producto: %s\nPlataforma: %s\nPrecio: %.2f\n\n",
                        nombre,
                        plataforma,
                        precio);


                // Guardamos en consola
                sprintf(linea,
                        "Producto: %s\nPlataforma: %s\nPrecio: %.2f\n\n",
                        nombre,
                        plataforma,
                        precio);

                strcat(memoria->productos, linea);
            }
        }


        fprintf(reporte,
                "====================================\n");

        fprintf(reporte,
                "TOTAL MENSUAL: %.2f\n",
                total_ventas);


        strcat(memoria->productos,
               "====================================\n");


        sprintf(linea,
                "TOTAL MENSUAL: %.2f\n",
                total_ventas);

        strcat(memoria->productos, linea);


        fclose(ventas);

        fclose(reporte);


        strcpy(memoria->respuesta,
               "Reporte mensual generado correctamente");
    }
}

// FUNCION PARA AGREGAR PRODUCTO
else if(accion_local == AGREGAR_PRODUCTO_ADMIN)
{

    FILE *archivo;

    char linea[300];

    int id_existente;

    char nombre[100];

    char plataforma[100];

    float precio;

    int stock;

    int repetido = 0;


    // Abrimos archivo para lectura
    archivo = fopen("productos.txt", "r");


    // Verificamos archivo
    if(archivo == NULL)
    {

        strcpy(memoria->respuesta,
               "Error al abrir productos.txt");
    }
    else
    {
	// Revisamos si el ID ya existe
while(fgets(linea, sizeof(linea), archivo))
{

    sscanf(linea,
           "%d;%[^;];%[^;];%f;%d",
           &id_existente,
           nombre,
           plataforma,
           &precio,
           &stock);


    // Verificamos ID repetido
    if(id_existente == memoria->producto.id)
    {

        repetido = 1;

        break;
    }
}
// Si el ID ya existe
if(repetido)
{

    fclose(archivo);

    strcpy(memoria->respuesta,
           "El ID ya existe");
}
else
{
	// Reabrimos archivo en modo append
	fclose(archivo);

	archivo = fopen("productos.txt", "a");
        // Guardamos producto
        fprintf(archivo,
                "%d;%s;%s;%.2f;%d\n",
                memoria->producto.id,
                memoria->producto.nombre,
                memoria->producto.plataforma,
                memoria->producto.precio,
                memoria->producto.stock);


        fclose(archivo);


        strcpy(memoria->respuesta,
               "Producto agregado correctamente");
    }
	}
}

// FUNCION PARA MODIFICAR STOCK
else if(accion_local == MODIFICAR_STOCK)
{

    FILE *archivo;

    FILE *temp;


    int id;

    int stock;

    float precio;


    char nombre[100];

    char plataforma[100];

    char linea[300];


    int encontrado = 0;


    // Abrimos productos
    archivo = fopen("productos.txt", "r");


    // Creamos temporal
    temp = fopen("temp.txt", "w");


    // Verificamos archivos
    if(archivo == NULL || temp == NULL)
    {

        strcpy(memoria->respuesta,
               "Error al abrir productos");
    }
    else
    {

        // Recorremos archivo
        while(fgets(linea, sizeof(linea), archivo))
        {

            // Leemos producto
            sscanf(linea,
                   "%d;%[^;];%[^;];%f;%d",
                   &id,
                   nombre,
                   plataforma,
                   &precio,
                   &stock);


            // Verificamos ID
            if(id == memoria->producto.id)
            {

                encontrado = 1;


                // Escribimos producto actualizado
                fprintf(temp,
                        "%d;%s;%s;%.2f;%d\n",
                        id,
                        nombre,
                        plataforma,
                        precio,
                        memoria->producto.stock);
            }
            else
            {

                // Reescribimos producto normal
                fprintf(temp,
                        "%d;%s;%s;%.2f;%d\n",
                        id,
                        nombre,
                        plataforma,
                        precio,
                        stock);
            }
        }


        fclose(archivo);

        fclose(temp);


        // Si encontramos producto
        if(encontrado)
        {

            remove("productos.txt");

            rename("temp.txt", "productos.txt");


            strcpy(memoria->respuesta,
                   "Stock modificado correctamente");
        }
        else
        {

            remove("temp.txt");


            strcpy(memoria->respuesta,
                   "Producto no encontrado");
        }
    }
}

// FUNCION PARA ELIMINAR PRODUCTO
else if(accion_local == ELIMINAR_PRODUCTO)
{

    FILE *archivo;

    FILE *temp;


    int id;

    int stock;

    float precio;


    char nombre[100];

    char plataforma[100];

    char linea[300];


    int encontrado = 0;


    // Abrimos archivo productos
    archivo = fopen("productos.txt", "r");


    // Creamos temporal
    temp = fopen("temp.txt", "w");


    // Verificamos archivos
    if(archivo == NULL || temp == NULL)
    {

        strcpy(memoria->respuesta,
               "Error al abrir productos");
    }
    else
    {

        // Recorremos archivo
        while(fgets(linea, sizeof(linea), archivo))
        {

            // Leemos producto
            sscanf(linea,
                   "%d;%[^;];%[^;];%f;%d",
                   &id,
                   nombre,
                   plataforma,
                   &precio,
                   &stock);


            // Verificamos si es el producto a eliminar
            if(id == memoria->producto.id)
            {

                encontrado = 1;
            }
            else
            {

                // Reescribimos producto
                fprintf(temp,
                        "%d;%s;%s;%.2f;%d\n",
                        id,
                        nombre,
                        plataforma,
                        precio,
                        stock);
            }
        }


        fclose(archivo);

        fclose(temp);


        // Si encontramos producto
        if(encontrado)
        {

            remove("productos.txt");

            rename("temp.txt", "productos.txt");


            strcpy(memoria->respuesta,
                   "Producto eliminado correctamente");
        }
        else
        {

            remove("temp.txt");


            strcpy(memoria->respuesta,
                   "Producto no encontrado");
        }
    }
}

// FUNCION PARA MODIFICAR MI CUENTA
else if(accion_local == MODIFICAR_MI_CUENTA)
{

    FILE *archivo;

    FILE *temp;


    char linea[300];

    char usuario_archivo[100];

    char password_archivo[100];

    char correo_archivo[100];


    int encontrado = 0;


    char password_cifrada[100];


    // Ciframos nueva password
    contrasenacif(password_local, password_cifrada);


    // Abrimos archivo original
    archivo = fopen("usuarios.txt", "r");


    // Creamos temporal
    temp = fopen("temp.txt", "w");


    // Verificamos archivos
    if(archivo == NULL || temp == NULL)
    {

        strcpy(memoria->respuesta,
               "Error al abrir archivos");
    }
    else
    {

        // Recorremos archivo
        while(fgets(linea, sizeof(linea), archivo))
        {

            // Detectamos usuario
            if(strstr(linea, "Usuario: ") != NULL)
            {

                sscanf(linea,
                       "Usuario: %[^\n]",
                       usuario_archivo);


                // Leemos password
                fgets(linea, sizeof(linea), archivo);
                sscanf(linea,
                       "Password: %[^\n]",
                       password_archivo);


                // Leemos correo
                fgets(linea, sizeof(linea), archivo);
                sscanf(linea,
                       "Correo: %[^\n]",
                       correo_archivo);


                // Leemos separador
                fgets(linea, sizeof(linea), archivo);


                // Verificamos usuario actual
                if(strcmp(usuario_archivo,
                          memoria->mensaje) == 0)
                {

                    encontrado = 1;


                    // Escribimos nuevos datos
                    fprintf(temp,
                            "Usuario: %s\n",
                            usuario_local);

                    fprintf(temp,
                            "Password: %s\n",
                            password_cifrada);

                    fprintf(temp,
                            "Correo: %s\n",
                            correo_local);

                    fprintf(temp,
                            "------------------------------------\n");
                }
                else
                {

                    // Reescribimos usuario normal
                    fprintf(temp,
                            "Usuario: %s\n",
                            usuario_archivo);

                    fprintf(temp,
                            "Password: %s\n",
                            password_archivo);

                    fprintf(temp,
                            "Correo: %s\n",
                            correo_archivo);

                    fprintf(temp,
                            "------------------------------------\n");
                }
            }
        }


        fclose(archivo);

        fclose(temp);


        // Si encontramos usuario
        if(encontrado)
        {

            remove("usuarios.txt");

            rename("temp.txt", "usuarios.txt");


            strcpy(memoria->respuesta,
                   "Cuenta modificada correctamente");
        }
        else
        {

            remove("temp.txt");


            strcpy(memoria->respuesta,
                   "Usuario no encontrado");
        }
    }
}

else
    {
        printf("Operacion desconocida\n"); //por si eligen otra fuera del rango
    }

	    // HACEMOS UP AL SEMAFORO DEL CLIENTE PARA DESPERTARLO YA QUE TERMINAMOS DE ESCRIBIR LA RESPUESTA
	    struct sembuf op_v[] = {{0, 1, 0}};
	    semop(semaforo_cliente, op_v, 1);

	    pthread_exit(NULL); //cerramos la funcion del hilo
	}


void down(int semid) //funcion para hacser down al semaforo
{
    struct sembuf op_p[] = {{0, -1, 0}};
    semop(semid,op_p,1);
}

void up(int semid) //funcion para hacer up al semaforo
{
    struct sembuf op_v[] = {{0, 1, 0}};
    semop(semid,op_v,1);
}



int Crea_semaforo(key_t llave,int valor_inicial) //funcion para crear semaforo que proporciono el profe
{
    int semid=semget(llave,1,IPC_CREAT|PERMISOS);

    if(semid==-1)
    {
        return -1;
    }

    semctl(semid,0,SETVAL,valor_inicial);

    return semid;
}

// FUNCION PARA BUSCAR PRODUCTOS POR NOMBRE
void BuscarProducto(struct Datos *memoria)
{
    FILE *archivo; // Variable para manejar el archivo

    char linea[200]; // Guardamos cada linea del archivo

    struct Productos p_actual; // Variable temporal para almacenar cada producto

    char resultados[2000] = ""; // Aqui guardaremos todos los productos encontrados

    char busqueda[100];

    strcpy(busqueda, memoria->productos);

    // Abrimos el archivo de productos en modo lectura
    archivo = fopen("productos.txt", "r");

    // limpia la memoria
    memset(memoria->productos, 0, sizeof(memoria->productos));

    // Validamos si el archivo se abrio correctamente
    if (archivo == NULL)
    {
        strcpy(memoria->productos, "No se pudo abrir el archivo de productos.\n");

        strcpy(memoria->respuesta, "ERROR");

        return; // Salimos de la funcion
    }

    // Recorremos todo el archivo linea por linea
    while (fgets(linea, sizeof(linea), archivo))
    {

        // Leemos los datos de cada producto del archivo
        sscanf(linea,
               "%d;%[^;];%[^;];%f;%d",
               &p_actual.id,
               p_actual.nombre,
               p_actual.plataforma,
               &p_actual.precio,
               &p_actual.stock);


        // Buscamos coincidencias entre el nombre escrito
        // por el usuario y el nombre del producto
        if (strstr(p_actual.nombre, busqueda) != NULL)
        {

            char temp[300]; // Variable temporal para formatear el texto


            // Formateamos la informacion del producto encontrado
            sprintf(temp,
                    "ID: %d | %s | %s | $%.2f | Stock: %d\n",
                    p_actual.id,
                    p_actual.nombre,
                    p_actual.plataforma,
                    p_actual.precio,
                    p_actual.stock);


            // Agregamos el producto encontrado al resultado final
            strcat(resultados, temp);
        }
    }


    // Cerramos el archivo
    fclose(archivo);


    // Validamos si no se encontro ningun producto
    if (strlen(resultados) == 0)
    {
        strcpy(memoria->productos, "Producto no encontrado.\n");
    }
    else
    {

        // Guardamos todos los resultados encontrados
        // en la memoria compartida
        strcpy(memoria->productos, resultados);
    }


    // Enviamos respuesta positiva al cliente
    strcpy(memoria->respuesta, "OK");
}

// FUNCION PARA MOSTRAR TODOS LOS USUARIOS REGISTRADOS
void VerUsuarios(struct Datos *memoria)
{
    FILE *archivo; // Variable para manejar el archivo

    char linea[300]; // Guardamos cada linea del archivo

    char resultados[3000] = ""; // Aqui guardaremos todos los usuarios


    // Abrimos el archivo de usuarios en modo lectura
    archivo = fopen("usuarios.txt", "r");


    // Verificamos si el archivo existe
    if(archivo == NULL)
    {
        strcpy(memoria->productos, "No se pudo abrir usuarios.txt\n");

        strcpy(memoria->respuesta, "ERROR");

        return;
    }


    // Titulo del reporte
    strcat(resultados, "\n========== USUARIOS REGISTRADOS ==========\n\n");


    // Recorremos todo el archivo linea por linea
    while(fgets(linea, sizeof(linea), archivo))
    {

        // Agregamos cada linea al resultado final
        strcat(resultados, linea);
    }


    // Cerramos el archivo
    fclose(archivo);


    // Mandamos los resultados al cliente
    strcpy(memoria->productos, resultados);


    // Indicamos que todo salio bien
    strcpy(memoria->respuesta, "OK");
}

//para evitar que haya basura
void limpiar_IPC(int senal) {
    printf("\nCerrando servidor de forma limpia...\n");
    
    // Borramos los semáforos del sistema operativo
    semctl(semaforo_servidor, 0, IPC_RMID);
    semctl(semaforo_cliente, 0, IPC_RMID);
    
    // Desconectamos y borramos la memoria compartida
    shmdt(memoria);
    shmctl(shmid, IPC_RMID, NULL);
    
    printf("Memoria Compartida y Semaforos eliminados exitosamente de la RAM.\n");
    exit(0);
}


int main(int argc, char const *argv[])
{
    // Vinculamos la señal de Ctrl+C (SIGINT) a nuestra nueva función de limpieza
    signal(SIGINT, limpiar_IPC);

    //SEMAFORO
    key_t llave; //llave para el semaforo
    key_t llave_cliente; //llave nueva para el semaforo del cliente

    llave= ftok("archivo", 'k'); //se necesita crear "archivo" para que funcione
    llave_cliente = ftok("archivo", 'c'); //creamos una subllave diferente con el caracter 'c'

    if (llave==-1 || llave_cliente == -1) //por si ocurre un error al crear la llave
    {
        printf("Error al crear la llave\n");
    }

    // Inicializamos por separado ambos semáforos
    semaforo_servidor = Crea_semaforo(llave, 0); //llamamos a la funcion para crear el semaforo y lo guardamos
    semaforo_cliente = Crea_semaforo(llave_cliente, 0); //creamos el semaforo que usara el cliente para congelarse

    printf("Servidor Listo\n");


    //MEMORIA COMPARTIDA

    shmid=shmget(llave, sizeof(struct Datos), IPC_CREAT|PERMISOS); //Asignacion de la memoria compartida a shmid, con tamano de la estructura de datos

    if(shmid == -1){ //por si ocurre un error al crear la memoria compartida
        printf("Error al crear memoria\n");
        exit(1);
    }

    memoria= (struct Datos *) shmat(shmid, NULL, 0); //Conexion a la memoria

    if(memoria == (void*) -1){ //por si ocurre un error
        printf("Error al conectarse a la memoria\n");
        exit(1);
    }


    //funcion del servidor
    while(1){

        printf("\nEsperando cliente...\n"); //mostramos que estamos esperando a un cliente

        down(semaforo_servidor); //hacemos down en el semaforo, para dejar el proceso "suspendido"

        pthread_t hilo; //declaramos el hilo

        pthread_create(&hilo,NULL,Hilo,NULL); //creamos el hilo

        // === MODIFICACIÓN CLAVE ===
        // Para evitar problemas de carrera concurrentes mientras se lee/escribe en caliente,
        // dejamos que el hilo tome posesión e independice su ciclo de vida antes de reiniciar.
        usleep(50000); // Pequeña pausa de 50ms para asegurar el arranque limpio del hilo
        
        pthread_detach(hilo); //ejecutar el hilo de forma independiente
    }

    return 0;
}
