#include "driver.hpp"


mmmpsr::MimiumDriver::~MimiumDriver()
{

}

mmmpsr::MimiumDriver::parse(std::string &str){
    scanner.reset();
   try
   {
      scanner = std::make_unique<mmmpsr::MimiumScanner>( str );
   }
      catch( std::bad_alloc &ba )
   {
      std::cerr << "Failed to allocate scanner: (" <<
         ba.what() << "), exiting!!\n";
      exit( EXIT_FAILURE );
   }
    parser.reset();
   try
   {
      parser = std::make_unique<mmmpsr::MimiumParser>( *scanner,*this );
   }
      catch( std::bad_alloc &ba )
   {
      std::cerr << "Failed to allocate parser: (" << 
         ba.what() << "), exiting!!\n";
      exit( EXIT_FAILURE );
   }
      const int accept( 0 );
   if( parser->parse() != accept )
   {
      std::cerr << "Parse failed!!\n";
   }
    return ;
}