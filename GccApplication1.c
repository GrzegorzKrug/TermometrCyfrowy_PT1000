/*
 * GccApplication1.c
 *
 * Created: 2016-05-08 21:16:15
 * Author : Twój Komputer nie mój
 */

#include <avr/io.h>
#include <avr/interrupt.h>
volatile unsigned char tablica[] = { 0b00111111,0b00000111,0b01011011,0b01001111,0b01100110,0b01101101,0b01111101,0b00100111,0b01111111,0b01101111 }; 			// tablica znaków od 0 do 9 ,minus 
volatile unsigned char wys = 0;				// zmienna cechujaca wyswietlacz
volatile unsigned char cyfra[4] = { 0,0,0,0 };		// cyfry do wyświetlenia
volatile unsigned char stop = 0;				// zmienna do pauzowania programu
volatile unsigned char n = 0;				// zmienna do zliczania przerwan
volatile unsigned char licz = 0;				// zmienna warunkujaca liczenie trwania while
volatile unsigned char znak = 0;				// 0 ujemna, 1 dodatnia
volatile unsigned int czas_int = 0;		// czas_deintegracji oczekiwana ilość miesci się od 2857 do 10000
volatile unsigned int czas_deint = 0;	// czas_deintegracji oczekiwana ilość miesci się od 2857 do 10000

int main(void)
{
	double temperatura = 0;
	double poprzednia = 0;	//temperatura z poprzedniego cyklu
	int temp = 0;		//od temporary, tymczasowa zmienna, do ułatwienia obliczeń
	double rezystancja = 0;
	double napiecie = 0;
	DDRB = 0b11000000;
	DDRC = 0xFF;
	DDRD = 0xFF;

	sei();
	timer_init();

	while (1) 	//główna pętla programowa
	{
		napiecie = odczytaj_napiecie();
		rezystancja = napiecie / 0.001;				//źródło prądowe daje 1mA
		temperatura = przelicz_ohm_na_cels(rezystancja);
		temperatura = (1.2 * temperatura + poprzednia * 0.8) / 2;	// wagowe uśrednianie temperatury

		temp = (int)(temperatura * 10);
		if (znak == 1)						// 1 dodatni, 0 ujemny
		{
			cyfra[0] = tablica[(unsigned char)(temp / 10000)];
			cyfra[1] = tablica[(unsigned char)(temp / 1000)];
			cyfra[2] = tablica[(unsigned char)(temp / 100)];
			cyfra[3] = tablica[(unsigned char)(temp / 10)];
		}
		else
		{
			cyfra[0] = tablica[10];	// znak minus
			cyfra[1] = tablica[(unsigned char)(temp / 1000)];
			cyfra[2] = tablica[(unsigned char)(temp / 100)];
			cyfra[3] = tablica[(unsigned char)(temp / 10)];
		}

		cyfra[2] += 0b10000000;		// dopisanie przecinka, zawsze jest dokładność +- 0.1

		poprzednia = temperatura;	// zapamietanie temperatury to nastepnego uśredniania
	}; // ==============KONIEC WHILE
}

double przelicz_ohm_na_cels(double RT)
{
	double R0 = 1000;
	double T;
	T = (RT - R0) / (3.90802 / 1000 + 5.802 / 10000000);
	return T;
}

void timer_init()
{
	TIMSK0 = (1 << TOIE0);			//przerwanie od przepelnienia
	TCCR0A = (1 << WGM01);			//tryb ctc
	TCCR0B = (1 << CS01) | (1 << CS00);		//preskaler 256
	OCR0A = 255;

	TIMSK1 = (1 << TOIE1);			//przerwanie od przepelnienia
	TCCR1A = (1 << WGM12);			//tryb ctc
	TCCR1B = (1 << CS11);			//preskaler 8 , żeby cpu zdążył obsłyżyć przerwanie
	OCR1A = 100;				//to nam daje okres 50ns 
}

void czekaj()
{	//funkcja zależna od przerwania, czeka 66ms
	stop = 1;
	while (stop == 1);
}

double odczytaj_napiecie()
{
	double napiecie = 0;
	unsigned char output = 0;
	czas_deint = 0;
	czas_int = 1320;			//bo 50ns to jedno przepelnienie, a okres Tint=66ms

	//==========faza 1			zerowanie przetwornika
	PORTB = 0b01000000;			// A=0; B=1;
	czekaj();				// czekamy, okres wynosi 66ms,

	//==========faza 2			Integracja, odczytanie znaku
	PORTB = 0b10000000;			// A=1; B=0;	
	czekaj();				//czekaj 66ms

	//==========faza 3			Deintegracja
	output = PINB0;			//odczytanie znaku przed zmianą sygnału sterującego	
	PORTB = 0b11000000;		// A=1; B=1;
	while (PIN0 == output)
	{
		licz = 1;			// czekaj Tint=66ms
	};
	licz = 0;				//zatrzymaj liczenie

	//==========faza 4				wyłączenie przetwarzania
	PORTB = 0b00000000;// A=0; B=0;

	//==========zakonczenie
	napiecie = 1.025 * (double)czas_deint / (double)czas_int;
	// zmiane jednostek czasu mozna pominac, bo sie uproszaczą
	znak = output;
	// zapis znaku do pamieci na koncu, zeby nie zakłócić podczas obliczeń i czekania wyświetlania
	return napiecie;
}


ISR(TIMER0_OVF_vect)
// przerwanie odpowiadają za cykliczne wyświetlanie aktualnej temperatury i czekanie 66ms
{
	n++;
	wys++;				// dodawanie
	if (wys > 4)wys = 1;		//warunek zapewnia nam zmiane wartosci od 1 do 4
	if (n >= 64)			// zliczanie 66ms
	{
		n = 0;
		stop = 0;
	};
	// ustawienie znaku i klucza
	if (wys == 1)
	{
		PORTC = wys;
		PORTD = cyfra[0];
	}
	else
	{
		if (wys == 2)
		{
			PORTC = wys;
			PORTD = cyfra[1];
		}
		else
		{
			if (wys == 3)
			{
				PORTC = wys;
				PORTD = cyfra[2];
			}
			else
			{
				if (wys == 4)
				{
					PORTC = wys;
					PORTD = cyfra[3];
				}
			}
		}
	}
}

ISR(TIMER1_OVF_vect)
// jedno przerwanie 50ns, uzywane tylko do liczenia czasu o malym okresie
{
	if (licz == 1) czas_deint++;
}