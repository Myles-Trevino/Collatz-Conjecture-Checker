/*
	Copyright Myles Trevino
	Licensed under the Apache License, Version 2.0
	http://www.apache.org/licenses/LICENSE-2.0
*/


#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <gmp/gmp.h>


// Check if the number meets the Collatz conjecture.
void collatz_conjecture(mpz_t start_number)
{
	mpz_t number;
	mpz_init_set(number, start_number);

	while (true)
	{
		// If we have reached 1, break.
		if (mpz_cmp_ui(number, 1) == 0) break;

		// If the number is even, divide by two.
		if (mpz_even_p(number)) mpz_divexact_ui(number, number, 2);

		// If the number is odd, triple it and add one.
		else
		{
			mpz_mul_ui(number, number, 3);
			mpz_add_ui(number, number, 1);
		}
	}

	mpz_clear(number);
}


// The procedure run on each thread.
void thread_procedure(mpz_t thread_start_number, unsigned long iterations_per_thread)
{
	for (unsigned i{}; i < iterations_per_thread; ++i)
	{
		collatz_conjecture(thread_start_number);
		mpz_add_ui(thread_start_number, thread_start_number, 1);
	}
}


// Gets an unsigned 64-bit integer input of at least 1.
uint64_t get_int64(const std::string& prompt)
{
	while (true)
	{
		// Get the input.
		std::cout << prompt << ": ";
		std::string input;
		std::getline(std::cin, input);

		// Try to parse the input.
		uint64_t result{ std::strtoull(input.c_str(), nullptr, 10) };
		bool is_numeric{ input.find_first_not_of("0123456789") == std::string::npos };
		if (is_numeric && (result != ULLONG_MAX) && (result > 0)) return result;

		// Otherwise, retry.
		std::cout << "Invalid input.\n";
	}
}


// Main.
int main()
{
	const unsigned recommended_thread_count{ std::thread::hardware_concurrency() / 2 };
	std::cout << "Collatz Conjecture Checker by Myles Trevino\n\n---\n\n";

	// Get the thread count.
	const uint64_t thread_count{ get_int64("Thread Count (" +
		std::to_string(recommended_thread_count) + " Recommended)") };

	// Get the iterations per thread.
	const uint64_t iterations_per_thread{ get_int64(
		"Iterations Per Thread (1000000 Recommended)") };

	// Get the number to start at.
	mpz_t batch_start_number;
	std::string input;

	while (true)
	{
		std::cout << "Start number (Up to 2^68 has been checked as of 2020): ";
		std::getline(std::cin, input);
		mpz_init_set_str(batch_start_number, input.c_str(), 10);
		if (mpz_cmp_ui(batch_start_number, 0) > 0) break;
		std::cout << "Invalid input.\n";
	}

	mpz_t batch_end_number;
	mpz_init_set(batch_end_number, batch_start_number);

	// Print the settings.
	const uint64_t batch_size{ thread_count * iterations_per_thread };
	std::cout << "\n---\n\nUsing " << thread_count << " threads.\n"
		<< "Using " << iterations_per_thread << " iterations per thread.\n"
		<< "The batch size is " << batch_size << ".\n";

	gmp_printf("Starting at %Zd.\n\n---\n\n", batch_start_number);

	// Check.
	while (true)
	{
		mpz_add_ui(batch_end_number, batch_end_number, batch_size);
		gmp_printf("Trying %Zd - %Zd...", batch_start_number, batch_end_number);

		// Launch the threads.
		const auto start_time{ std::chrono::high_resolution_clock::now() };

		std::vector<std::thread> threads;
		mpz_t thread_start_number;
		mpz_init_set(thread_start_number, batch_start_number);

		for (unsigned i{}; i < thread_count; ++i)
		{
			threads.emplace_back(std::thread(thread_procedure,
				thread_start_number, iterations_per_thread));

			mpz_add_ui(thread_start_number, thread_start_number, iterations_per_thread);
		}

		mpz_clear(thread_start_number);

		// Wait for all the threads to complete.
		for (std::thread& thread : threads) thread.join();

		// Continue to the next batch.
		const auto delta{ std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::high_resolution_clock::now() - start_time).count() };

		std::cout << " Passed (" << delta << "ms).\n";
		mpz_add_ui(batch_start_number, batch_start_number, batch_size);
	}

	mpz_clear(batch_end_number);
	mpz_clear(batch_start_number);
}
