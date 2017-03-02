/*
    Данный пример демонстрирует работу с модулем LTR11.
    По умолчанию идет работа с первым слотом первого крейта, но это можно изменить
        через параметры командной строки:
    ltr11_recv  slot crate_serial srvip
    где:
        slot         - номер слота (от 1 до 16)
        crate_serial - серийный номер крейта (если крейтов больше одного)
        srvip        - ip-адрес LTR-сервера (если он запущен на другой машине)
    Параметры опциональны: можно указать только слот, слот и серийный номер крейта или все

    Пример принимает данные от 4-х каналов с частотой АЦП 400 КГц.
    На экране отображается только по первому отсчету каждого принятого блока.

    Пользователь может изменить настройки на свои, поменяв заполнение полей
    структуры перед запуском сбора.

    Сбор идет до нажатия любой клавиши на Windows или  CTRL+C на Linux

    Сборка в VisualStudio:
    Для того чтобы собрать проект в Visual Studio, измените путь к заголовочным файлам
    (Проект (Project) -> Свойства (Properties) -> Свойства конфигурации (Configuration Properties)
    -> С/С++ -> Общие (General) -> Дополнительные каталоги включения (Additional Include Directories))
    на нужный в зависимаости от пути установки библиотек (ltr11api.h  и остальные заголовочные
    файлы должны находится в поддиректории ltr/include относительно указанного пути)
    и измените путь к .lib файлам на <путь установки библиотек>/lib/msvc
    (Проект (Project) -> Свойства (Properties) -> Свойства конфигурации (Configuration Properties) ->
    Компоновщик (Linker) -> Общие (General) -> Дополнительные каталоги библиотек (Additional Library Directories)).

    Внимание!: Если Вы собираете проект под Visual Studio и у Вас некорректно
    отображается вывод русских букв, то нужно изменить кодировку:
    выберите Файл (File) -> Дополнительные параметры сохранения (Advanced Save Options)...
    и в поле Кодировка (Encoding) выберите Юникод (UTF8, с сигнатурой)/Unicode (UTF-8 with signature)
    и сохраните изменения в файле. А также следует убедится, что в настройках
    консоли стоит шрифт с поддержкой русского языка (например Consolas).
*/


#include "ltr/include/ltr11api.h"
/* остальные заголовочные файлы */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>


/* Количество отсчетов на канал, принмаемых за раз */
#define RECV_BLOCK_CH_SIZE  4096*8
/* таймаут на ожидание данных при приеме (без учета времени преобразования) */
#define RECV_TOUT  4000

typedef struct {
    int slot;
    const char *serial;
    DWORD addr;
} t_open_param;


/* признак необходимости завершить сбор данных */
static int f_out = 0;

/* Обработчик сигнала завершения для Linux */
static void f_abort_handler(int sig) {
    f_out = 1;
}

/* Разбор параметров командной строки. Если указано меньше, то используются
 * значения по умолчанию:
 * 1 параметр - номер слота (от 1 до 16)
 * 2 параметр - серийный номер крейта
 * 3 параметр - ip-адрес сервера */
static int f_get_params(int argc, char** argv, t_open_param* par) {
    int err = 0;
    par->slot = LTR_CC_CHNUM_MODULE1;
    par->serial = "";
    par->addr = LTRD_ADDR_DEFAULT;


    if (argc > 1)
        par->slot = atoi(argv[1]);
    if (argc > 2)
        par->serial = argv[2];
    if (argc > 3) {
        int a[4],i;
        if (sscanf(argv[3], "%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3])!=4) {
            fprintf(stderr, "Неверный формат IP-адреса!!\n");
            err = -1;
        }

        for (i=0; (i < 4) && !err; i++) {
            if ((a[i]<0) || (a[i] > 255)) {
                fprintf(stderr, "Недействительный IP-адрес!!\n");
                err = -1;
            }
        }

        if (!err) {
            par->addr = (a[0] << 24) | (a[1]<<16) | (a[2]<<8) | a[3];
        }
    }
    return err;
}



/*-----------------------------------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
    TLTR11 hltr11;
    INT  err;
    t_open_param par;

    struct sigaction sa;
    /* В ОС Linux устанавливаем свой обработчик на сигнал закрытия,
       чтобы завершить сбор корректно */
    sa.sa_handler = f_abort_handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);

    err = f_get_params(argc, argv, &par);

    if (!err) {
        LTR11_Init(&hltr11);              /* инициализация дескриптора модуля */
    
        /* открытие канала связи с модулем, установленным в слот 1 */
        err = LTR11_Open(&hltr11, par.addr, LTRD_PORT_DEFAULT, par.serial, par.slot);
    }
    if (err != LTR_OK) {
        fprintf(stderr, "Не удалось установить связь с модулем. Ошибка %d (%s)\n",
                err, LTR11_GetErrorString(err));
    } else {
        /* получение информации о модуле из flash-памяти */
        err = LTR11_GetConfig(&hltr11);
        if (err != LTR_OK) {
            fprintf(stderr, "Не удалось прочитать информацию о модуле. Ошибка %d: %s\n", err,
                    LTR11_GetErrorString(err));
        } else {  /* конфигурация получена успешно */
            /* вывод информации о модуле */
            printf( "Информация о модуле:\n"
                    "  Название модуля  : %s\n"
                    "  Серийный номер   : %s\n"
                    "  Версия прошивки  : %u.%u\n",
                       hltr11.ModuleInfo.Name, hltr11.ModuleInfo.Serial,
                      (hltr11.ModuleInfo.Ver >> 8) & 0xFF, hltr11.ModuleInfo.Ver & 0xFF);
            fflush(stdout);


	    /*TODO: Добавить код для установки требуемых папраметров работы модуля*/    
	

	    /* задание параметров работы модуля */
            /* режим старта сбора данных - внутренний */
            hltr11.StartADCMode = LTR11_STARTADCMODE_INT;
            /* режим синхронизации АПЦ - внутренний */
            hltr11.InpMode      = LTR11_INPMODE_INT;
            /* количество логических каналов - 4 */
            hltr11.LChQnt       = 4;
            /* таблица управления логическими каналами */
            /* диапазон - 10В, режим - 16-канальный, физический канал - 1 */
            hltr11.LChTbl[0] = LTR11_CreateLChannel(0, LTR11_CHMODE_DIFF, LTR11_CHRANGE_10000MV);
            /* диапазон - 2.5В, режим - измерение собственного нуля, физический канал - 2 */
            hltr11.LChTbl[1] = LTR11_CreateLChannel(1, LTR11_CHMODE_ZERO, LTR11_CHRANGE_2500MV);
            /* диапазон - 0.6В, режим - 32-канальный (каналы 1..16), физический канал - 3 */
            hltr11.LChTbl[2] = LTR11_CreateLChannel(2, LTR11_CHMODE_COMM, LTR11_CHRANGE_625MV);
            /* диапазон - 0.15В, режим - 32-канальный (каналы 17..32), физический канал - 25 */
            hltr11.LChTbl[3] = LTR11_CreateLChannel(24, LTR11_CHMODE_COMM, LTR11_CHRANGE_156MV);
            /* режим сбора данных */
            hltr11.ADCMode = LTR11_ADCMODE_ACQ;
            /* частота дискретизации - 400 кГц. Подбираем нужные делители */
            LTR11_FindAdcFreqParams(400000, &hltr11.ADCRate.prescaler,
                                    &hltr11.ADCRate.divider, NULL);
            
            /* передаем настройки в модуль */
            err = LTR11_SetADC(&hltr11);
            if (err != LTR_OK) {
                fprintf(stderr, "Не удалось установить настройки модуля. Ошибка %d: %s\n", err,
                        LTR11_GetErrorString(err));
            }
        }

        if (err == LTR_OK) {
            DWORD recvd_blocks=0;
            INT recv_data_cnt = RECV_BLOCK_CH_SIZE * hltr11.LChQnt;
            DWORD  *rbuf = (DWORD*)malloc(recv_data_cnt*sizeof(rbuf));
            double *data = (double *)malloc(recv_data_cnt*sizeof(data[0]));
            if ((rbuf==NULL) || (data==NULL)) {
                fprintf(stderr, "Ошибка выделения памяти!\n");
                err = LTR_ERROR_MEMORY_ALLOC;
            } else {
                /* запуск сбора данных */
                err = LTR11_Start(&hltr11);
                if (err != LTR_OK) {
                    fprintf(stderr, "Не удалось запустить сбор данных. Ошибка %d: %s\n", err,
                            LTR11_GetErrorString(err));
                } else {
                    INT stop_err = 0;
                    printf("Сбор данных запущен. Для останова нажмите %s\n",
#ifdef _WIN32
                           "любую клавишу"
#else
                           "CTRL+C"
#endif
                           );
                    fflush(stdout);

                    while (!f_out && (err==LTR_OK)) {
                        INT recvd;
                        /* в таймауте учитываем время выполнения самого преобразования*/
                        DWORD tout = RECV_TOUT + (DWORD)(RECV_BLOCK_CH_SIZE/hltr11.ChRate + 1);
                        /* получение данных от LTR11 */
                        recvd = LTR11_Recv(&hltr11, rbuf, NULL, recv_data_cnt, tout);
                        /* Значение меньше нуля соответствуют коду ошибки */
                        if (recvd<0) {
                            err = recvd;
                            fprintf(stderr, "Ошибка приема данных. Ошибка %d:%s\n",
                                    err, LTR11_GetErrorString(err));
                        } else if (recvd!=recv_data_cnt) {
                            fprintf(stderr, "Принято недостаточно данных. Запрашивали %d, приняли %d\n",
                                    recv_data_cnt, recvd);
                            err = LTR_ERROR_RECV_INSUFFICIENT_DATA;
                        } else {
                            /* сохранение принятых и обработанных данных в буфере */
                            err = LTR11_ProcessData(&hltr11, rbuf, data, &recvd, TRUE, TRUE);
                            if (err!=LTR_OK) {
                                fprintf(stderr, "Ошибка обработки данных. Ошибка %d:%s\n",
                                        err, LTR11_GetErrorString(err));
                            } else {
                                INT ch;
                                recvd_blocks++;
                                /* выводим по первому слову на канал */
                                printf("Блок %4d: ", recvd_blocks);

                                for (ch=0; ch < hltr11.LChQnt; ch++) {
                                    printf("%8.2f", data[ch]);

                                    if (ch==(hltr11.LChQnt-1)) {
                                        printf("\n");
                                    } else {
                                        printf(",  ");
                                    }
                                }
                                fflush(stdout);
                            }
                        }
#ifdef _WIN32
                        /* проверка нажатия клавиши для выхода */
                        if (err==LTR_OK) {
                            if (_kbhit())
                                f_out = 1;
                        }
#endif
                    } //while (!f_out && (err==LTR_OK))


                    /* останавливаем сбор данных */
                    stop_err = LTR11_Stop(&hltr11);
                    if (stop_err!=LTR_OK) {
                        fprintf(stderr, "Не удалось остановить сбор данных. Ошибка %d: %s\n", err,
                            LTR11_GetErrorString(stop_err));
                    } else {
                        if (err==LTR_OK)
                            err = stop_err;
                    }
                }

                free(rbuf);
                free(data);
            }
        }
    }

    if (LTR11_IsOpened(&hltr11) == LTR_OK) {
        /* закрытие канала связи с модулем */
        LTR11_Close(&hltr11);
    }
        
    return err;
}


