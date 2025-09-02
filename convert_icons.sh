#!/bin/bash
# Script to convert SVG icons to PNG for better OpenGL compatibility

ICON_DIR="./icons"
TARGET_SIZE="32"  # 32x32 pixels for UI buttons
OUTPUT_DIR="./icons/png"

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "Converting SVG icons to PNG format..."
echo "Target size: ${TARGET_SIZE}x${TARGET_SIZE} pixels"
echo "==============================================="

# Convert each SVG file to PNG
for svg_file in "$ICON_DIR"/*.svg; do
    if [ -f "$svg_file" ]; then
        filename=$(basename "$svg_file" .svg)
        output_file="$OUTPUT_DIR/${filename}.png"
        
        echo "Converting: $filename.svg -> $filename.png"
        
        # Use Inkscape to convert SVG to PNG with high quality
        inkscape "$svg_file" \
            --export-type=png \
            --export-filename="$output_file" \
            --export-width=$TARGET_SIZE \
            --export-height=$TARGET_SIZE \
            --export-background-opacity=0 \
            2>/dev/null
        
        if [ $? -eq 0 ]; then
            echo "  ✓ Success: $output_file"
        else
            echo "  ✗ Failed: $svg_file"
        fi
    fi
done

# Also copy existing PNG files to the output directory and resize them if needed
echo ""
echo "Processing existing PNG files..."
echo "================================"

for png_file in "$ICON_DIR"/*.png; do
    if [ -f "$png_file" ]; then
        filename=$(basename "$png_file")
        output_file="$OUTPUT_DIR/$filename"
        
        echo "Processing: $filename"
        
        # Get current image dimensions
        dimensions=$(identify -format "%wx%h" "$png_file" 2>/dev/null)
        
        if [ $? -eq 0 ]; then
            width=$(echo $dimensions | cut -d'x' -f1)
            height=$(echo $dimensions | cut -d'x' -f2)
            
            if [ "$width" != "$TARGET_SIZE" ] || [ "$height" != "$TARGET_SIZE" ]; then
                echo "  Resizing from ${dimensions} to ${TARGET_SIZE}x${TARGET_SIZE}"
                convert "$png_file" -resize "${TARGET_SIZE}x${TARGET_SIZE}" -background transparent -gravity center -extent "${TARGET_SIZE}x${TARGET_SIZE}" "$output_file"
            else
                echo "  Already correct size, copying..."
                cp "$png_file" "$output_file"
            fi
            
            if [ $? -eq 0 ]; then
                echo "  ✓ Success: $output_file"
            else
                echo "  ✗ Failed: $png_file"
            fi
        else
            echo "  ✗ Could not read dimensions: $png_file"
        fi
    fi
done

echo ""
echo "Icon conversion complete!"
echo "========================"
echo "Converted icons are in: $OUTPUT_DIR"
ls -la "$OUTPUT_DIR"
