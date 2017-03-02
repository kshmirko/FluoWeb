/* 
    Данный пример демонстрирует работу с модулем LTR210.
    По умолчанию идет работа с первым слотом первого крейта, но это можно изменить
        через параметры командной строки:
    ltr210_recv  slot crate_serial srvip
    где:
        slot         - номер слота (от 1 до 16)
        crate_serial - серийный номер крейта (если крейтов больше одного)
        srvip        - ip-адрес LTR-сервера (если не локальный)
    Параметры опциональны: можно указать только слот, слот и серийный номер крейта или все

    Пример принимает данные от двух каналов в режиме периодического покадрового сбора,
    отображает на экране первые 2 слова кадра, а полный кадр сохраняется в
    файл Frame<номер кадра>.txt (по файлу на каждый кадр) в текстовом виде.

    Пользователь может изменить настройки на свои, поменяв заполнение полей
    структуры конфигурации.

    Сбор идет до нажатия любой клавиши на Windows или  CTRL+C на Linux

    Сборка в VisualStudio:
    Для того чтобы собрать проект в Visual Studio, измените путь к заголовочным файлам
    (Проект (Project) -> Свойства (Properties) -> Свойства конфигурации (Configuration Properties)
    -> С/С++ -> Общие (General) -> Дополнительные каталоги включения (Additional Include Directories))
    на нужный в зависимаости от пути установки библиотек (ltr210api.h  и остальные заголовочные
    файлы должны находится в поддиректории ltr/include относительно указанного пути)
    и измените путь к .lib файлам на <путь установки библиотек>/lib/msvc
    (Проект (Project) -> Свойства (Properties) -> Свойства конфигурации (Configuration Properties) ->
    Компоновщик (Linker) -> Общие (General) -> Дополнительные каталоги библиотек (Additional Library Directories)).

    Внимание!: Если Вы собираете проект под Visual Studio и у Вас некорректно
    отображается вывод русских букв, то нужно изменить кодировку:
    выберите Файл (File) -> Дополнительные параметры сохранения (Advanced Save Options)...
    и в поле Кодировка (Encoding) выберите Юникод (UTF8, с сигнатурой)/Unicode (UTF-8 with signature)
    и сохраните изменения в файле.
*/


#include "ltr/include/ltr210api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#define FRAME_SAVE_TO_FILE
//#define FRAME_DISPLAY
/* таймаут на прием кадра в мс */
#define RECV_TOUT       10000
/* Если за данное время не придет ни одного слова от модуля, то считаем его неисправным */
#define KEEPALIVE_TOUT  10000
#define CONFIG_NAME "config.ltr210"

/* если определено, то кадры полностью сохраняются в файл */
#define FRAME_SAVE_TO_FILE

/* путь к прошивке ПЛИС. Пустая строка => используем встроенную прошивку */
#define LTR210_FPGA_FIRM_FILE ""

/* признак необходимости завершить сбор данных */
static int f_out = 0;


typedef struct
{
  int slot;
  const char *serial;
  DWORD addr;
} t_open_param;

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

/* индикация процесса загрузки */
static void APIENTRY
f_firm_upd_cb (void *cb_data, TLTR210 * hnd, DWORD done_size, DWORD full_size)
{
  printf (".");
  fflush (stdout);
}

int
ReadLTR210Config(const char* conf_name, TLTR210* hltr210, int *len, int *req_blocks, int *zero_offset)
{
  FILE *f;

  if((f=fopen(conf_name, "rt"))!=NULL)
    {
      int ok; 
      ok = fscanf(f, "SYNC MODE: %d\n", &hltr210->Cfg.SyncMode) == 1;
      if(!ok)
	{
	  perror("Error reading config file line 1");
	  exit(1);
	}
      ok = fscanf(f, "FLAGS: %d\n",&hltr210->Cfg.Flags)==1;
      if(!ok)
	{
	  perror("Error reading config file line 2.");
	  exit(2);
	}
      double hist_time, meas_time;
      ok = fscanf(f, "HISTORY TIME (mks): %lf\n", &hist_time)==1;
      if(!ok)
	{
	  perror("Error reading config file line 3.");
	  exit(3);
	}
      ok = fscanf(f, "MEASUREMENTS TIME (mks): %lf\n", &meas_time)==1;
      if(!ok)
	{
	  perror("Error reading config file line 4.");
	  exit(4);
	}
      double adc_freq;
      ok = fscanf(f, "FREQUENCY: %lf Hz\n", &adc_freq)==1;
      if(!ok)
	{
	  perror("Error reading config file line 5.");
	  exit(5);
	}
      ok=LTR210_FillAdcFreq(&hltr210->Cfg, adc_freq, 0, NULL)==LTR_OK;
      if(!ok)
	{
	  perror("Error setting ADC frequency.");
	  exit(6);
	}

      ok = fscanf(f, "MEAS ZERO OFFSET: %d\n", zero_offset) == 1;
      if(!ok)
	{
	  perror("Error reading cinfig file line 6.");
	  exit(7);
	}

      ok = fscanf(f, "REQUIRED BLOCKS: %d\n", req_blocks) == 1;
      
      if(!ok)
	{
	  perror("Error reading config file line 7.");
	  exit(8);
	}
      
      int ch, en, rng, md;
      ok = fscanf(f, "CH[%d].Enabled: %d\n", &ch, &en) == 2;
      if(!ok)
	{
	  perror("Error reading config file line 8.");
	  exit(9);
	}
      ok = fscanf(f, "CH[%d].Range: %d\n", &ch, &rng) == 2;
      if(!ok)
	{
	  perror("Error reading config file line 9.");
	  exit(10);
	}
      ok = fscanf(f, "CH[%d].Mode: %d\n", &ch, &md) == 2;
      if(!ok)
	{
	  perror("Error reading config file line 10.");
	  exit(11);
	}

      hltr210->Cfg.Ch[ch].Enabled = en;
      hltr210->Cfg.Ch[ch].Range = rng;
      hltr210->Cfg.Ch[ch].Mode = md;

      ok = fscanf(f, "CH[%d].Enabled: %d\n", &ch, &en) == 2;
      if(!ok)
	{
	  perror("Error reading config file line 11.");
	  exit(12);
	}
      ok = fscanf(f, "CH[%d].Range: %d\n", &ch, &rng) == 2;
      if(!ok)
	{
	  perror("Error reading config file line 12.");
	  exit(13);
	}
      ok = fscanf(f, "CH[%d].Mode: %d\n", &ch, &md) == 2;
      if(!ok)
	{
	  perror("Error reading config file line 13.");
	  exit(14);
	}

      hltr210->Cfg.Ch[ch].Enabled = en;
      hltr210->Cfg.Ch[ch].Range = rng;
      hltr210->Cfg.Ch[ch].Mode = md;

      int NHist = ceil(adc_freq*hist_time*1e-6);
      *len = ceil(adc_freq*meas_time*1e-6);

      hltr210->Cfg.FrameSize = *len;
      hltr210->Cfg.HistSize = NHist;

      double frame_freq;
      ok = fscanf(f, "FRAME FREQ: %lf Hz\n", &frame_freq)==1;
      if(!ok)
	{
	  perror("Error reading config file line 14.");
	  exit(15);
	}

      if(hltr210->Cfg.SyncMode==LTR210_SYNC_MODE_PERIODIC)
	{
	  double set_ffreq;
	  ok = LTR210_FillFrameFreq (&hltr210->Cfg, frame_freq, &set_ffreq)==LTR_OK;
	  printf("%lf", set_ffreq);
	  if(!ok)
	    {
	      perror("Error setting frame frequency");
	      exit(16);
	    }
	}

    }
  return LTR_OK;
}    


static int
f_save_to_file (int frame_num, double *data, int size, int ch_cnt)
{
  char fname[40];
  FILE *f;
  int i, ch;
  int err = 0;

  /* сохраняем данные в обычный текстовый файл. Каждому каналу свой столбец */
  sprintf (fname, "ltr210_%05d.sav", frame_num);
  f = fopen (fname, "wt");
  if (f != NULL)
    {
      for (i = 0; (i < size) && !err; i += ch_cnt)
	{
	  for (ch = 0; (ch < ch_cnt) && !err; ch++)
	    {
	      if (fprintf (f, "%10.5f\t", data[i + ch]) <= 0)
		err = 1;
	    }

	  if (fprintf (f, "\n") <= 0)
	    err = 1;
	}
      fclose (f);

      if (err)
	{
	  fprintf (stderr, "Ошибка записи в файл!\n");
	}
    }
  else
    {
      printf ("Ошибка создания файла!\n");
    }
  return err;
}


/* Обработчик сигнала завершения для Linux */
static void
f_abort_handler (int sig)
{
  f_out = 1;
}

int
main (int argc, char **argv)
{
  INT err = LTR_OK;
  TLTR210 hltr210;
  t_open_param par;
  struct sigaction sa;
  /* В ОС Linux устанавливаем свой обработчик на сигнал закрытия,
     чтобы завершить сбор корректно */
  sa.sa_handler = f_abort_handler;
  sigaction (SIGTERM, &sa, NULL);
  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGABRT, &sa, NULL);

  err = f_get_params (argc, argv, &par);

  if (!err)
    {
      LTR210_Init (&hltr210);
      /* устанавливаем соединение с модулем */
      err =
	LTR210_Open (&hltr210, par.addr, LTRD_PORT_DEFAULT, par.serial,
		     par.slot);
      if (err)
	{
	  fprintf (stderr,
		   "Не удалось установить связь с модулем. Ошибка %d (%s)\n",
		   err, LTR210_GetErrorString (err));
	  
	}
      else
	{
	  /* после открытия модуля доступна информация, кроме версии прошивки ПЛИС */
	  printf ("Модуль открыт успешно!\n");
	  printf ("  Название модуля    = %s\n",
		  hltr210.ModuleInfo.Name);
	  printf ("  Серийный номер     = %s\n",
		  hltr210.ModuleInfo.Serial);
	  printf ("  Версия PLD         = %d\n",
		  hltr210.ModuleInfo.VerPLD);
	  fflush (stdout);


	  /* Проверяем, загружена ли прошивка. Если нет - загружаем */
	  if (LTR210_FPGAIsLoaded (&hltr210) != LTR_OK)
	    {
	      printf
		("Начало записи прошивки модуля\n");
	      err =
		LTR210_LoadFPGA (&hltr210, LTR210_FPGA_FIRM_FILE,
				 f_firm_upd_cb, NULL);
	      if (err != LTR_OK)
		{
		  fprintf (stderr,
			   "Не удалось загрузить прошивку ПЛИС. Ошибка %d (%s)\n",
			   err, LTR210_GetErrorString (err));
		}
	      else
		{
		  printf
		    ("\nПрошивка ПЛИС загружена успешно.\n");
		}
	    }

	  if (err == LTR_OK)
	    {
	      printf ("Версия прошивки ПЛИС = %d\n",
		      hltr210.ModuleInfo.VerFPGA);
	    }

	  if (err == LTR_OK)
	    {
	      DWORD *wrds = NULL;
	      double *data = NULL;

	      int nPoints, nReqBlocks, bZeroOffset;

	      /* ------------- Установка настроек модуля --------------- */
	      err = ReadLTR210Config(CONFIG_NAME, &hltr210, &nPoints, &nReqBlocks, &bZeroOffset);

	      /* настройки каналов АЦП */
	      /*hltr210.Cfg.Ch[0].Enabled = 1;
	      hltr210.Cfg.Ch[0].Range = LTR210_ADC_RANGE_10;
	      hltr210.Cfg.Ch[0].Mode = LTR210_CH_MODE_ACDC;
	      hltr210.Cfg.Ch[1].Enabled = 1;
	      hltr210.Cfg.Ch[1].Range = LTR210_ADC_RANGE_0_5;
	      hltr210.Cfg.Ch[1].Mode = LTR210_CH_MODE_AC;

	      hltr210.Cfg.SyncMode = LTR210_SYNC_MODE_PERIODIC;*/

	      /* Размер кадра */
	      //hltr210.Cfg.FrameSize = 10000;
	      /* Размер предыстории */
	      //hltr210.Cfg.HistSize = 100;




	      /* Разрешаем посылку сигнала жизни и автоматическую
	         приостановку записи */
	      //hltr210.Cfg.Flags = LTR210_CFG_FLAGS_KEEPALIVE_EN |
		//LTR210_CFG_FLAGS_WRITE_AUTO_SUSP;


	      /* Устанавливаем максимальную частоту отсчетов - 10 МГц */
	      //err = LTR210_FillAdcFreq (&hltr210.Cfg, 10000000., 0, NULL);
	      /* Устанавливаем частоту кадров 1 Гц (кадр в секунду) */
	      //if ((err == LTR_OK) && bltr210.Cfg.SyncMode == LTR210_SYNC_MODE_PERIODIC)
		//err = LTR210_FillFrameFreq (&hltr210.Cfg, 1., NULL);
	      /* Записываем настройки в модуль */
	      if (err == LTR_OK)
		{
		  err = LTR210_SetADC (&hltr210);
		  if (err != LTR_OK)
		    {
		      fprintf (stderr,
			       "Не удалось установить настройки АЦП: Ошибка %d (%s)\n",
			       err, LTR210_GetErrorString (err));
		    }
		}


	      if ((err == LTR_OK) && bZeroOffset)
		{
		  /* Измеряем уход нуля для дополнительной коррекции */
		  err = LTR210_MeasAdcZeroOffset (&hltr210, 0);
		  if (err != LTR_OK)
		    {
		      fprintf (stderr,
			       "Не удалось измерить смещение нуля АЦП: Ошибка %d (%s)\n",
			       err, LTR210_GetErrorString (err));
		    }
		}


	      if (err == LTR_OK)
		{
		  /* выделяем память под принимаемые и обработанные слова */
		  wrds =
		    (DWORD *) malloc (hltr210.State.RecvFrameSize *
				      sizeof (wrds[0]));
		  data =
		    (double *) malloc (hltr210.State.RecvFrameSize *
				       sizeof (data[0]));

		  if ((wrds == NULL) || (data == NULL))
		    {
		      fprintf (stderr,
			       "Ошибка выделения памяти!\n");
		      err = LTR_ERROR_MEMORY_ALLOC;
		    }
		}

	      if (err == LTR_OK)
		{
		  err = LTR210_Start (&hltr210);
		  if (err != LTR_OK)
		    {
		      fprintf (stderr,
			       "Не удалось запустить сбор данных! Ошибка %d (%s)\n",
			       err, LTR210_GetErrorString (err));
		    }
		  else
		    {
		      printf
			("Сбор данных запущен. Для останова нажмите %s\n",
#ifdef _WIN32
			 "любую клавишу"
#else
			 "CTRL+C"
#endif
			);
		      fflush (stdout);

		    }
		}

	      static int frame_num=0;
	      while (!f_out && (err == LTR_OK) && (frame_num<nReqBlocks))
		{
		  DWORD evt;
		  INT recv_cnt = 0;
		  

		  /* Ожидаем прихода данных от модуля */
		  if (err == LTR_OK)
		    err = LTR210_WaitEvent (&hltr210, &evt, NULL, 100);
		  if (err == LTR_OK)
		    {
		      /* Анализ, что за событие произошло */
		      switch (evt)
			{
			case LTR210_RECV_EVENT_SOF:
			  /* Пришел новый кадр. Для простоты принимаем
			   * его за один Recv, однако при желании можно
			   * разбить прием на несколько блоков */
			  recv_cnt = LTR210_Recv (&hltr210, wrds, NULL,
						  hltr210.State.RecvFrameSize,
						  RECV_TOUT);
			  /* Значение меньше нуля соответствуют коду ошибки */
			  if (recv_cnt < 0)
			    {
			      err = recv_cnt;
			      fprintf (stderr,
				       "Ошибка при приеме кадра: Ошибка %d (%s)\n",
				       err, LTR210_GetErrorString (err));
			    }
			  else if (recv_cnt <
				   (INT) hltr210.State.RecvFrameSize)
			    {
			      fprintf (stderr,
				       "Принято меньше слов, чем было в кадре! запрашивали %d, приняли %d\n",
				       hltr210.State.RecvFrameSize, recv_cnt);
			      err = LTR_ERROR_RECV_INSUFFICIENT_DATA;
			    }
			  else
			    {
			      TLTR210_FRAME_STATUS frame_st;
			      /* переводим данные в Вольты с коррекцией АЧХ и нуля */
			      err =
				LTR210_ProcessData (&hltr210, wrds, data,
						    &recv_cnt,
						    LTR210_PROC_FLAG_VOLT |
						    LTR210_PROC_FLAG_AFC_COR |
						    LTR210_PROC_FLAG_ZERO_OFFS_COR,
						    &frame_st, NULL);
			      if (err != LTR_OK)
				{
				  printf
				    ("Ошибка обработки данных! Ошибка %d (%s)\n",
				     err, LTR210_GetErrorString (err));
				}
			      else
				{
				  int i;
				  frame_num++;
				  /* на экран выводим первые два отсчета,а
				   * полностью кадр сохраняем в файл */
#ifdef FRAME_DISPLAY
				  
				  printf
				    ("Успешно приняли кадр %d: первые отсчеты: ",
				     frame_num);
				  for (i = 0; (i < 2) && (i < recv_cnt); i++)
				    {
				      printf (" %.4f В", data[i]);
				    }
				  printf ("\n");
				  fflush (stdout);
#endif
#ifdef FRAME_SAVE_TO_FILE
				  f_save_to_file (frame_num, data, recv_cnt,
						  (hltr210.Cfg.Ch[0].Enabled
						   && hltr210.Cfg.Ch[1].
						   Enabled) ? 2 : 1);
#endif
				}
			    }
			  break;
			case LTR210_RECV_EVENT_TIMEOUT:
			  if (hltr210.Cfg.
			      Flags & LTR210_CFG_FLAGS_KEEPALIVE_EN)
			    {
			      /* если ничего не пришло, то проверяем, не превышен ли таймаут на ожидание
			       * сигналов жизни */
			      DWORD interval;
			      err =
				LTR210_GetLastWordInterval (&hltr210,
							    &interval);
			      if (err == LTR_OK)
				{
				  if (interval > KEEPALIVE_TOUT)
				    {
				      fprintf (stderr,
					       "Не было периодических статусов от модуля за заданный интервал. Модуль не исправен!\n");
				      err =
					LTR210_ERR_KEEPALIVE_TOUT_EXCEEDED;
				    }
				}
			      else
				{
				  fprintf (stderr,
					   "Не удалось получить интервал времени с момента приема последнего слова. Ошибка %d (%s)\n",
					   err, LTR210_GetErrorString (err));
				}
			    }
			default:
			  break;
			}
		    }
		  else
		    {
		      fprintf (stderr,
			       "Ошибка при ожидании данных от модуля! Ошибка %d (%s)\n",
			       err, LTR210_GetErrorString (err));
		    }

		}		// while(!f_out && (err==LTR_OK))

	      /* останавливаем запись данных модулем, если она была запущена */
	      if (hltr210.State.Run)
		{
		  INT stoperr = LTR210_Stop (&hltr210);
		  if (stoperr == LTR_OK)
		    {
		      printf
			("Сбор остановлен успешно!\n");
		    }
		  else
		    {
		      fprintf (stderr,
			       "Сбор остановлен с ошибкой. Ошибка %d (%s)\n",
			       err, LTR210_GetErrorString (err));
		      if (err == LTR_OK)
			err = stoperr;
		    }
		}

	      free (wrds);
	      free (data);
	    }
	}
      LTR210_Close (&hltr210);
    }


  return err;
}
