#!/usr/bin/env ruby
require 'rubygems'
require 'RMagick'

def get_sectors(filename)
  puts "Reading rooms..."
  state = 0
  dir_state = 0
  vnum = 0
  sectors = {}
  starting_vnum = nil
  
  File.open(filename, "r") do |infile|
    while (line = infile.gets)
      case state
      when 0
        vnum = line.scan(/^#(\d+)(\s)*$/)[0][0].to_i
        starting_vnum = vnum if starting_vnum.nil?
        state = 1
      when 1
        state = 2
      when 2
        state = 3 if line =~ /^~(\s)*$/
      when 3
        # sector type is 3rd
        sector = line.scan(/^(\d+) (\d+) (\d+) (\d+)(\s)*$/)[0][2].to_i
        sectors[vnum] = sector        
        state = 4
      when 4
        if line =~ /^S(\s)*$/
          state = 0
        else
          case dir_state
          when 0
            dir_state = 1
          when 1
            dir_state = 2
          when 2
            dir_state = 3
          when 3
            dir_state = 0
          end
        end
      end
    end
  end
  
  puts "#{sectors.size} rooms loaded"
  
  return sectors, starting_vnum
end

SECT_INSIDE                   = 0  
SECT_CITY                     = 1  
SECT_FIELD                    = 2  
SECT_FOREST                   = 3  
SECT_HILLS                    = 4  
SECT_MOUNTAIN                 = 5  
SECT_WATER_SWIM               = 6  
SECT_WATER_NOSWIM             = 7  
SECT_NO_GROUND                = 8  
SECT_UNDERWATER               = 9  
SECT_UNDERWATER_GR            = 10 
SECT_FIREPLANE                = 11 
SECT_OCEAN                    = 12 
SECT_UNDRWLD_WILD             = 13 
SECT_UNDRWLD_CITY             = 14 
SECT_UNDRWLD_INSIDE           = 15 
SECT_UNDRWLD_WATER            = 16 
SECT_UNDRWLD_NOSWIM           = 17 
SECT_UNDRWLD_NOGROUND         = 18 
SECT_AIR_PLANE                = 19 
SECT_WATER_PLANE              = 20 
SECT_EARTH_PLANE              = 21 
SECT_ETHEREAL                 = 22 
SECT_ASTRAL                   = 23 
SECT_DESERT                   = 24 
SECT_ARCTIC                   = 25 
SECT_SWAMP                    = 26 
SECT_UNDRWLD_MOUNTAIN         = 27  
SECT_UNDRWLD_SLIME            = 28  
SECT_UNDRWLD_LOWCEIL          = 29  
SECT_UNDRWLD_LIQMITH          = 30  
SECT_UNDRWLD_MUSHROOM         = 31  
SECT_CASTLE_WALL              = 32  
SECT_CASTLE_GATE              = 33 
SECT_CASTLE                   = 34 
SECT_NEG_PLANE                = 35 
SECT_PLANE_OF_AVERNUS         = 36  
SECT_ROAD                     = 37
SECT_SNOWY_FOREST             = 38

SECTOR_SYMBOLS = {
SECT_INSIDE            => "^",
SECT_CITY              => "+",
SECT_FIELD             => ".",
SECT_FOREST            => "*",
SECT_HILLS             => "^",
SECT_MOUNTAIN          => "M",
SECT_WATER_SWIM        => "r",
SECT_WATER_NOSWIM      => " ",
SECT_NO_GROUND         => "", 
SECT_UNDERWATER        => "", 
SECT_UNDERWATER_GR     => "", 
SECT_FIREPLANE         => " ",
SECT_OCEAN             => " ",
SECT_UNDRWLD_WILD      => ".",
SECT_UNDRWLD_CITY      => "*",
SECT_UNDRWLD_INSIDE    => ".",
SECT_UNDRWLD_WATER     => " ",
SECT_UNDRWLD_NOSWIM    => " ",
SECT_UNDRWLD_NOGROUND  => "", 
SECT_AIR_PLANE         => "", 
SECT_WATER_PLANE       => "", 
SECT_EARTH_PLANE       => " ",
SECT_ETHEREAL          => "", 
SECT_ASTRAL            => "R",
SECT_DESERT            => ".",
SECT_ARCTIC            => "M",
SECT_SWAMP             => "*",
SECT_UNDRWLD_MOUNTAIN  => "M",
SECT_UNDRWLD_SLIME     => "*",
SECT_UNDRWLD_LOWCEIL   => ",",
SECT_UNDRWLD_LIQMITH   => " ",
SECT_UNDRWLD_MUSHROOM  => "o",
SECT_CASTLE_WALL       => " ",
SECT_CASTLE_GATE       => "T",
SECT_CASTLE            => "O",
SECT_NEG_PLANE         => " ",
SECT_PLANE_OF_AVERNUS  => " ",
SECT_ROAD              => "+" 
}

BR = "&+R"
DR = "&+r"
BW = "&+W"
DW = "&+w"
BY = "&+Y"
DY = "&+y"
BB = "&+B"
DB = "&+b"
BC = "&+C"
DC = "&+c"
BL = "&+L"
DL = "&+l"
BM = "&+M"
DM = "&+m"
BG = "&+G"
DG = "&+g"

COLORS = {
BR => "#FF6666",
DR => "#CC0000",
BW => "#FFFFFF",
DW => "#CCCCCC",
BY => "#FFFF66",
DY => "#CCCC00",
BB => "#6666FF",
DB => "#0000CC",
BC => "#66FFFF",
DC => "#00CCCC",
BL => "#666666",
DL => "#000000",
BM => "#FF66FF",
DM => "#CC00CC",
BG => "#66FF66",
DG => "#00CC00"
}

SECTOR_ANSI_COLORS = {
  SECT_INSIDE            => [DL,DL],
  SECT_CITY              => [DL,DL],
  SECT_FIELD             => [DG,DL],
  SECT_FOREST            => [DL,DG],
  SECT_HILLS             => [DY,DL],
  SECT_MOUNTAIN          => [DY,DL],
  SECT_WATER_SWIM        => [DL,DC],
  SECT_WATER_NOSWIM      => [BB,DB],
  SECT_NO_GROUND         => [DW,DL], 
  SECT_UNDERWATER        => [DW,DL], 
  SECT_UNDERWATER_GR     => [DW,DL], 
  SECT_FIREPLANE         => [BR,DR],
  SECT_OCEAN             => [BB,DB],
  SECT_UNDRWLD_WILD      => [DL,DM],
  SECT_UNDRWLD_CITY      => [DL,DW],
  SECT_UNDRWLD_INSIDE    => [DL,DM],
  SECT_UNDRWLD_WATER     => [BB,DB],
  SECT_UNDRWLD_NOSWIM    => [BB,DB],
  SECT_UNDRWLD_NOGROUND  => [DW,DL], 
  SECT_AIR_PLANE         => [DW,DL], 
  SECT_WATER_PLANE       => [BL,DL], 
  SECT_EARTH_PLANE       => [DW,DL],
  SECT_ETHEREAL          => [DW,DL], 
  SECT_ASTRAL            => [DW,DL],
  SECT_DESERT            => [BY,DY],
  SECT_ARCTIC            => [BC,DC],
  SECT_SWAMP             => [DL,DM],
  SECT_UNDRWLD_MOUNTAIN  => [BL,BL],
  SECT_UNDRWLD_SLIME     => [BG,DG],
  SECT_UNDRWLD_LOWCEIL   => [BM,DL],
  SECT_UNDRWLD_LIQMITH   => [BW,DW],
  SECT_UNDRWLD_MUSHROOM  => [BM,DL],
  SECT_CASTLE_WALL       => [BW,DW],
  SECT_CASTLE_GATE       => [BL,DW],
  SECT_CASTLE            => [BL,DW],
  SECT_NEG_PLANE         => [BL,DL],
  SECT_PLANE_OF_AVERNUS  => [BR,DL],
  SECT_ROAD              => [DL,DW]
}  

SECTOR_COLORS = {
  SECT_INSIDE            => "#E1E1E1",
  SECT_CITY              => "#454545",
  SECT_FIELD             => "#56725F",
  SECT_FOREST            => "#275746",
  SECT_HILLS             => "#AE7356",
  SECT_MOUNTAIN          => "#755446",
  SECT_WATER_SWIM        => "#2970A3",
  SECT_WATER_NOSWIM      => "#2970A3",
  SECT_NO_GROUND         => "#FFFFFF", 
  SECT_UNDERWATER        => "#095083", 
  SECT_UNDERWATER_GR     => "#196093", 
  SECT_FIREPLANE         => "#9E3134",
  SECT_OCEAN             => "#196093",
  SECT_UNDRWLD_WILD      => "#CC00CC",
  SECT_UNDRWLD_CITY      => "#E1E1E1",
  SECT_UNDRWLD_INSIDE    => "#E1E1E1",
  SECT_UNDRWLD_WATER     => "#196093",
  SECT_UNDRWLD_NOSWIM    => "#095083",
  SECT_UNDRWLD_NOGROUND  => "#222222", 
  SECT_AIR_PLANE         => "#66FFFF", 
  SECT_WATER_PLANE       => "#196093", 
  SECT_EARTH_PLANE       => "#D6B56A",
  SECT_ETHEREAL          => COLORS[DL], 
  SECT_ASTRAL            => COLORS[DL],
  SECT_DESERT            => "#D6B56A",
  SECT_ARCTIC            => "#FFFFFF",
  SECT_SWAMP             => "#AB86AE",
  SECT_UNDRWLD_MOUNTAIN  => COLORS[BL],
  SECT_UNDRWLD_SLIME     => COLORS[BG],
  SECT_UNDRWLD_LOWCEIL   => COLORS[BM],
  SECT_UNDRWLD_LIQMITH   => COLORS[BW],
  SECT_UNDRWLD_MUSHROOM  => "#AB86AE",
  SECT_CASTLE_WALL       => COLORS[BW],
  SECT_CASTLE_GATE       => COLORS[BL],
  SECT_CASTLE            => COLORS[BL],
  SECT_NEG_PLANE         => COLORS[BL],
  SECT_PLANE_OF_AVERNUS  => COLORS[BR],
  SECT_ROAD              => "#454545",
  SECT_SNOWY_FOREST      => COLORS[BW]
}  


def generate_map_image(map_width, map_height, square_size, starting_vnum, sectors, output_filename)
  puts "Generating image..."
  image = Magick::Image.new(map_width*square_size, map_height*square_size) do
    self.background_color = "#000"
  end

  vnum = starting_vnum
  map_height.times do |x|
    map_width.times do |y|
      vnum = starting_vnum + ( y * map_width) + x
      
      unless sectors[vnum].nil?
        square_size.times do |i|
          square_size.times do |j|
            image.pixel_color((x*square_size)+i,(y*square_size)+j, SECTOR_COLORS[sectors[vnum]])            
          end
        end
      end
    end
  end

  image.write(output_filename)
end

if ARGV.size != 4
  puts "dump_map_image makes a png of a duris map .wld file"
  puts "syntax: dump_map_image <filename.wld> <map size X> <map size Y> <size in pixels of map square>"
  exit
else
	sectors, starting_vnum = get_sectors(ARGV[0])
	generate_map_image(ARGV[1].to_i, ARGV[2].to_i, ARGV[3].to_i, starting_vnum, sectors, ARGV[0]+".png")
	puts ARGV[0]+".png generated."
end
