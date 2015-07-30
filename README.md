# L1sd0rker

Dorker, programa utilizado para automatizar pesquisas em buscadores, faz uso de alguns filtros.
 - Autor: Constantine - P0cL4bs Team - 07/2015.

### Help...

			 __                       __                 __
			/\ \       __            /\ \               /\ \
			\ \ \     /\_\    ____   \_\ \    ___   _ __\ \ \/'\      __   _ __
			 \ \ \  __\/\ \  /',__\  /'_` \  / __`\/\\`'__\ \ , <   /'__`\/\`'__\
			  \ \ \L\ \\ \ \/\__, `\/\ \L\ \/\ \L\ \ \ \/ \ \ \\`\ /\  __/\ \ \/
			   \ \____/ \ \_\/\____/\ \___,_\ \____/\ \_\  \ \_\ \_\ \____\\ \_\
				\/___/   \/_/\/___/  \/__,_ /\/___/  \/_/   \/_/\/_/\/____/ \/_/

							Coded by Constantine - Greatz for L1sbeth
								My GitHub: github.com/jessesilva
								P0cL4bs Team: github.com/P0cL4bs


		  -l  -  List of dorks.
		  -t  -  Number of threads (Default: 1).
		  -v  -  Top-level filter.
		  -p  -  Path to search (Default: 200 OK HTTP error).
			  '-> Output path file: DORK_FILE_NAME-paths.txt
		  -d  -  Data to search.
		  -c  -  Search engine (Default: all).
			  |-> 1: Bing
			  |-> 2: Google - NOT IMPLEMENTED
			  |-> 3: Hotbot - NOT IMPLEMENTED
			  '-> 4: DuckDuckGo - NOT IMPLEMENTED
		  -m  -  Save method (Default: all).
			  |-> 1: Extract domains (DORK_FILE_NAME-domains.txt).
			  '-> 2: Extract full URL (DORK_FILE_NAME-full_url.txt).

		  Examples...
			dorker -l dorks.txt -t 30 -m 1 -v .com.br
			dorker -l dorks.txt -t 30 -m 2 -v .gov
			dorker -l dorks.txt  -t 30 -v .gov,.com,.com.br
			dorker -l dorks-bing.txt -t 30 -m 1 -c 1 -v .br
					  -p /wp-login.php,/wp-admin.php,/wordpress/wp-login.php -d "wp-submit"
			dorker -l dorks-all.txt -t 100 -c 12