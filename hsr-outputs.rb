#!/usr/bin/env ruby
# gem install xrandr

require 'json'
require 'xrandr'

outputs = Xrandr::Control.new.outputs.select(&:connected).map do |o|
  x, y = o.position.split('x')
  width, height = o.resolution.split('x')
  { :name => o.name, :x => x, :y => y, :width => width, :height => height }
end

def print_csv(outputs)
  puts("name,x,y,width,height")
  outputs.each { |o| puts("#{o[:name]},#{o[:x]},#{o[:y]},#{o[:width]},#{o[:height]}") }
end

def print_json(outputs)
  puts outputs.to_json
end

def print_text(outputs)
  outputs.each_with_index do |o, i|
    puts('') if i > 0
    puts("name: #{o[:name]}")
    puts("\tx:\t#{o[:x]}")
    puts("\ty:\t#{o[:y]}")
    puts("\twidth:\t#{o[:width]}")
    puts("\theight:\t#{o[:height]}")
  end
end

if ARGV.first == '--csv'
  print_csv(outputs)
elsif ARGV.first == '--json'
  print_json(outputs)
elsif ARGV.first.nil? or ARGV.first == '--text'
  print_text(outputs)
else
  $stderr.puts("syntax: #$0 [<--text|--json|--csv>]")
end

