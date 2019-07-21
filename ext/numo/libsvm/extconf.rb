require 'mkmf'
require 'numo/narray'

$LOAD_PATH.each do |lp|
  if File.exist?(File.join(lp, 'numo/numo/narray.h'))
    $INCFLAGS = "-I#{lp}/numo #{$INCFLAGS}"
    break
  end
end

unless have_header('numo/narray.h')
  puts 'numo/narray.h not found.'
  exit(1)
end

if RUBY_PLATFORM =~ /mswin|cygwin|mingw/
  $LOAD_PATH.each do |lp|
    if File.exist?(File.join(lp, 'numo/libnarray.a'))
      $LDFLAGS = "-L#{lp}/numo #{$LDFLAGS}"
      break
    end
  end
  unless have_library('narray', 'nary_new')
    puts 'libnarray.a not found.'
    exit(1)
  end
end

if RUBY_PLATFORM =~ /linux/
  $INCFLAGS = "-I/usr/include/libsvm #{$INCFLAGS}"
end

unless have_header('svm.h')
  puts 'svm.h not found.'
  exit(1)
end

unless have_library('svm')
  puts 'libsvm not found.'
  exit(1)
end

create_makefile('numo/libsvm/libsvmext')
