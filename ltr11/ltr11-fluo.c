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
#include <math.h>
#include <sys/time.h>


/* Количество отсчетов на канал, принмаемых за раз */
#define RECV_BLOCK_CH_SIZE  4096*8
/* таймаут на ожидание данных при приеме (без учета времени преобразования) */
#define RECV_TOUT  4000
#define CONFIG_FILE "config.ltr11"
#define SAVE_FILE_NAME "ltr11_%05d.sav"

typedef struct
{
  int slot;
  const char *serial;
  DWORD addr;
} t_open_param;


/* признак необходимости завершить сбор данных */
static int f_out = 0;

/* Обработчик сигнала завершения для Linux */
static void
f_abort_handler (int sig)
{
  f_out = 1;
}

/* Разбор параметров командной строки. Если указано меньше, то используются
 * значения по умолчанию:
 * 1 параметр - номер слота (от 1 до 16)
 * 2 параметр - серийный номер крейта
 * 3 параметр - ip-адрес сервера */
static int
f_get_params (int argc, char **argv, t_open_param * par)
{
  int err = 0;
  par->slot = LTR_CC_CHNUM_MODULE1;
  par->serial = "";
  par->addr = LTRD_ADDR_DEFAULT;


  if (argc > 1)
    par->slot = atoi (argv[1]);
  if (argc > 2)
    par->serial = argv[2];
  if (argc > 3)
    {
      int a[4], i;
      if (sscanf (argv[3], "%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3]) != 4)
	{
	  fprintf (stderr,
		   "Неверный формат IP-адреса!!\n");
	  err = -1;
	}

      for (i = 0; (i < 4) && !err; i++)
	{
	  if ((a[i] < 0) || (a[i] > 255))
	    {
	      fprintf (stderr,
		       "Недействительный IP-адрес!!\n");
	      err = -1;
	    }
	}

      if (!err)
	{
	  par->addr = (a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
	}
    }
  return err;
}

int
ReadLTR11Config(const char* conf_name, TLTR11* hltr11, int *LEN, int *ReqBlocks)
{
  FILE *f=NULL;
  if((f=fopen(conf_name,"rt"))!=NULL)
    {
      int f_freq;
      int ok = fscanf(f, "START ADC MODE: %d\n", &hltr11->StartADCMode) == 1;
      if(!ok)
	{
	  perror("Error in line 1 of config file.");
	  exit(1);
	}
      ok = fscanf(f, "INPUT MODE: %d\n", &hltr11->InpMode) ==1;
      if(!ok)
	{
	  perror("Error in line 2 of config file.");
	  exit(2);
	}
      ok = fscanf(f, "ADC MODE: %d\n", &hltr11->ADCMode) == 1;
      if(!ok)
	{
	  perror("Error in line 3 of config file.");
	  exit(3);
	}
      ok = fscanf(f, "FREQUENCY: %d Hz\n",&f_freq) == 1; 
      if(!ok)
	{
	  perror("Error in line 4 of config file.");
	  exit(4);
	}

      ok = LTR11_FindAdcFreqParams(f_freq, &hltr11->ADCRate.prescaler, 
	  &hltr11->ADCRate.divider, NULL) == LTR_OK;
      if(!ok)
	{
	  perror("Error in setting frequency.");
	  exit(5);
	}

      ok = fscanf(f, "LOGICAL CHANNELS COUNT: %d\n", &hltr11->LChQnt) == 1;
      if(!ok)
	{
	  perror("Error in line 5 of config file.");
	  exit(5);
	}

      int k=0, mode, range;
      for(int i=0; i<hltr11->LChQnt; i++)
        {
	  ok =  fscanf(f, "CH[%d] = (mode = %d, range = %d)\n", &k, &mode, &range) == 3;
	  if(!ok)
	    {
	      perror("Error in logical channels configuration lines");
	      exit(i+6);
	    }
	  hltr11->LChTbl[i] = LTR11_CreateLChannel(k, mode, range);
	}
      double delay;
      ok = fscanf(f, "MEASUREMENTS TIME (mks): %lf\n", &delay)==1;
      if(!ok)
	{
	  perror("Error setting measurement time");
	  exit(hltr11->LChQnt+7);
	}

      *LEN = (int)ceil(((delay+16.0)*1e-6*f_freq)/hltr11->LChQnt);

      printf("delay = %lf, len = %d, ffreq = %d\n", delay, *LEN, f_freq);
      ok = fscanf(f, "REQUIRED BLOCKS : %d\n", ReqBlocks)==1;
      if(!ok)
	{
	  perror("Error reading number of required blocks");
	  exit(hltr11->LChQnt+8);
	}


      fclose(f);
    }
  return LTR_OK;
}


int
SaveDataToFile(int block, int nchs, int LEN, double data[])
{

  char fname[255];
  sprintf(fname, SAVE_FILE_NAME, block);
  FILE *f = NULL;

  if((f=fopen(fname, "wt"))!=NULL)
    {
      for(int irow=0; irow<LEN; irow++)
	{
	  for(int jcol=0; jcol<nchs; jcol++)
	    {
	      fprintf (f, "%8.3lf", data[(irow*nchs)+jcol]);
	      if(jcol==(nchs-1))
		{
		  fprintf(f, "\n");
		}
	      else
		{
		  fprintf(f, ", ");
		}
	    }
	}
      fclose(f);
      return LTR_OK;
    }
  return 19191;
}

/*-----------------------------------------------------------------------------------------------*/
int
main (int argc, char *argv[])
{
  TLTR11 hltr11;
  INT err;
  int LEN; // Длина получаемого массива данных на 1 канал
  int RequiredBlocks = 1;

  t_open_param par;

  struct sigaction sa;
  /* В ОС Linux устанавливаем свой обработчик на сигнал закрытия,
     чтобы завершить сбор корректно */
  sa.sa_handler = f_abort_handler;
  sigaction (SIGTERM, &sa, NULL);
  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGABRT, &sa, NULL);

  struct timeval tv1, tv2;

  err = f_get_params (argc, argv, &par);

  if (!err)
    {
      LTR11_Init (&hltr11);	/* инициализация дескриптора модуля */

      /* открытие канала связи с модулем, установленным в слот 1 */
      err =
	LTR11_Open (&hltr11, par.addr, LTRD_PORT_DEFAULT, par.serial,
		    par.slot);
    }
  if (err != LTR_OK)
    {
      fprintf (stderr,
	       "Не удалось установить связь с модулем. Ошибка %d (%s)\n",
	       err, LTR11_GetErrorString (err));
    }
  else
    {
      /* получение информации о модуле из flash-памяти */
      err = LTR11_GetConfig (&hltr11);
      if (err != LTR_OK)
	{
	  fprintf (stderr,
		   "Не удалось прочитать информацию о модуле. Ошибка %d: %s\n",
		   err, LTR11_GetErrorString (err));
	}
      else
	{			/* конфигурация получена успешно */
	  /* вывод информации о модуле */
	  printf ("Информация о модуле:\n"
		  "  Название модуля  : %s\n"
		  "  Серийный номер   : %s\n"
		  "  Версия прошивки  : %u.%u\n",
		  hltr11.ModuleInfo.Name, hltr11.ModuleInfo.Serial,
		  (hltr11.ModuleInfo.Ver >> 8) & 0xFF,
		  hltr11.ModuleInfo.Ver & 0xFF);
	  fflush (stdout);


	  /*TODO: Добавить код для установки требуемых папраметров работы модуля */

	  err = ReadLTR11Config(CONFIG_FILE, &hltr11, &LEN, &RequiredBlocks);

	  /* передаем настройки в модуль */
	  err = LTR11_SetADC (&hltr11);
	  if (err != LTR_OK)
	    {
	      fprintf (stderr,
		       "Не удалось установить настройки модуля. Ошибка %d: %s\n",
		       err, LTR11_GetErrorString (err));
	    }
	}

      if (err == LTR_OK)
	{
	  // Установка параметров измерений прошла успешно
	  // готовим память для сохранения результата измерений
	  DWORD recvd_blocks = 0;
	  	  INT recv_data_cnt = LEN * hltr11.LChQnt;
	  DWORD *rbuf = (DWORD *) malloc (recv_data_cnt * sizeof (rbuf));
	  double *data = (double *) malloc (recv_data_cnt * sizeof (data[0]));
	  if ((rbuf == NULL) || (data == NULL))
	    {
	      fprintf (stderr,
		       "Ошибка выделения памяти!\n");
	      err = LTR_ERROR_MEMORY_ALLOC;
	    }
	  else
	    {
	      /* запуск сбора данных */
	      err = LTR11_Start (&hltr11);
	      gettimeofday(&tv1, NULL);

	      if (err != LTR_OK)
		{
		  fprintf (stderr,
			   "Не удалось запустить сбор данных. Ошибка %d: %s\n",
			   err, LTR11_GetErrorString (err));
		}
	      else
		{
		  INT stop_err = 0;
		  printf
		    ("Сбор данных запущен. Для останова нажмите %s\n",
		     "CTRL+C"
		    );
		  fflush (stdout);

		  // Цикл полуения данных из модуля
		  // выполняем пока нет ошимки и флаг f_out не установлен
		  // либо пока не принято необходимое количество блоков
		  while (!f_out && (err == LTR_OK) && (recvd_blocks<RequiredBlocks))
		    {
		      INT recvd;
		      /* в таймауте учитываем время выполнения самого преобразования */
		      DWORD tout =
			RECV_TOUT +
			(DWORD) (LEN / hltr11.ChRate + 1);
		      
		      /* получение данных от LTR11 */
		      recvd =
			LTR11_Recv (&hltr11, rbuf, NULL, recv_data_cnt, tout);
		      gettimeofday(&tv2, NULL);
		      printf("Total measurement time in mks: %ld, %d\n", ((uint64_t)1000000*tv2.tv_sec+tv2.tv_usec)-((uint64_t)1000000*tv1.tv_sec+tv1.tv_usec), recvd);
		      fflush(stdout);
		      /* Значение меньше нуля соответствуют коду ошибки */
		      if (recvd < 0)
			{
			  err = recvd;
			  fprintf (stderr,
				   "Ошибка приема данных. Ошибка %d:%s\n",
				   err, LTR11_GetErrorString (err));
			}
		      else if (recvd != recv_data_cnt)
			{
			  fprintf (stderr,
				   "Принято недостаточно данных. Запрашивали %d, приняли %d\n",
				   recv_data_cnt, recvd);

			  err = LTR_ERROR_RECV_INSUFFICIENT_DATA;
			}
		      else
			{
			  /* сохранение принятых и обработанных данных в буфере */
			  err =
			    LTR11_ProcessData (&hltr11, rbuf, data, &recvd,
					       TRUE, TRUE);
			  gettimeofday(&tv2, NULL);
			  printf("Total measurement time in mks: %ld\n", ((uint64_t)1000000*tv2.tv_sec+tv2.tv_usec)-((uint64_t)1000000*tv1.tv_sec+tv1.tv_usec));
			  fflush(stdout);
			  if (err != LTR_OK)
			    {
			      fprintf (stderr,
				       "Ошибка обработки данных. Ошибка %d:%s\n",
				       err, LTR11_GetErrorString (err));
			    }
			  else
			    {
			      // Сохранение данных в файл
			      recvd_blocks++;

			      SaveDataToFile(recvd_blocks, hltr11.LChQnt, LEN, data);
			      gettimeofday(&tv2, NULL);
			      printf("Total measurement time in mks: %ld\n", ((uint64_t)1000000*tv2.tv_sec+tv2.tv_usec)-((uint64_t)1000000*tv1.tv_sec+tv1.tv_usec));
			      fflush(stdout);

			    }
			}

		    }		//while (!f_out && (err==LTR_OK))


		  /* останавливаем сбор данных */
		  stop_err = LTR11_Stop (&hltr11);
		  if (stop_err != LTR_OK)
		    {
		      fprintf (stderr,
			       "Не удалось остановить сбор данных. Ошибка %d: %s\n",
			       err, LTR11_GetErrorString (stop_err));
		    }
		  else
		    {
		      if (err == LTR_OK)
			err = stop_err;
		    }
		}

	      free (rbuf);
	      free (data);
	    }
	}
    }

  if (LTR11_IsOpened (&hltr11) == LTR_OK)
    {
      /* закрытие канала связи с модулем */
      LTR11_Close (&hltr11);
    }

  return err;
}
