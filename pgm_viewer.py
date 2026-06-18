#!/usr/bin/env python3
"""
PGM Image Viewer with Basic Pixel Erase Functionality

A simple GUI application to view PGM (Portable GrayMap) images
and erase pixels by clicking and dragging on them.

Features:
- Load PGM files (P2 ASCII and P5 binary formats)
- Display image in a window
- Erase pixels by clicking/dragging (sets them to black/0)
- Save modified image
- Zoom in/out
- Reset view
"""

import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from PIL import Image, ImageTk
import numpy as np


class PGMViewer:
    def __init__(self, root):
        self.root = root
        self.root.title("PGM Image Viewer")
        
        # Image data
        self.image = None
        self.image_array = None
        self.original_image = None
        self.photo_image = None
        
        # Canvas settings
        self.zoom_level = 1.0
        self.pan_x = 0
        self.pan_y = 0
        self.is_dragging = False
        self.last_x = 0
        self.last_y = 0
        self.is_erasing = False
        self.brush_size = 5
        
        # Create UI
        self.create_menu()
        self.create_toolbar()
        self.create_canvas()
        self.create_statusbar()
        
        # Bind events
        self.bind_events()
    
    def create_menu(self):
        """Create menu bar"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Open...", command=self.open_image, accelerator="Ctrl+O")
        file_menu.add_command(label="Save As...", command=self.save_image, accelerator="Ctrl+S")
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit, accelerator="Ctrl+Q")
        
        # Edit menu
        edit_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Edit", menu=edit_menu)
        edit_menu.add_command(label="Reset Image", command=self.reset_image, accelerator="Ctrl+R")
        edit_menu.add_command(label="Clear All", command=self.clear_all, accelerator="Ctrl+X")
        
        # View menu
        view_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="View", menu=view_menu)
        view_menu.add_command(label="Zoom In", command=self.zoom_in, accelerator="+")
        view_menu.add_command(label="Zoom Out", command=self.zoom_out, accelerator="-")
        view_menu.add_command(label="Fit to Window", command=self.fit_to_window, accelerator="Ctrl+0")
        
        # Help menu
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="About", command=self.show_about)
    
    def create_toolbar(self):
        """Create toolbar with controls"""
        toolbar = ttk.Frame(self.root)
        toolbar.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        # Open button
        open_btn = ttk.Button(toolbar, text="Open", command=self.open_image)
        open_btn.pack(side=tk.LEFT, padx=2)
        
        # Save button
        save_btn = ttk.Button(toolbar, text="Save", command=self.save_image)
        save_btn.pack(side=tk.LEFT, padx=2)
        
        # Separator
        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)
        
        # Erase mode toggle
        self.erase_var = tk.BooleanVar(value=True)
        erase_check = ttk.Checkbutton(toolbar, text="Erase Mode", variable=self.erase_var)
        erase_check.pack(side=tk.LEFT, padx=2)
        
        # Brush size
        ttk.Label(toolbar, text="Brush Size:").pack(side=tk.LEFT, padx=(10, 2))
        self.brush_size_var = tk.IntVar(value=5)
        brush_slider = ttk.Scale(toolbar, from_=1, to=50, variable=self.brush_size_var, 
                                  orient=tk.HORIZONTAL, length=100, command=self.update_brush_size)
        brush_slider.pack(side=tk.LEFT, padx=2)
        self.brush_size_label = ttk.Label(toolbar, text="5")
        self.brush_size_label.pack(side=tk.LEFT, padx=2)
        
        # Separator
        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)
        
        # Zoom controls
        zoom_in_btn = ttk.Button(toolbar, text="Zoom In (+)", command=self.zoom_in)
        zoom_in_btn.pack(side=tk.LEFT, padx=2)
        
        zoom_out_btn = ttk.Button(toolbar, text="Zoom Out (-)", command=self.zoom_out)
        zoom_out_btn.pack(side=tk.LEFT, padx=2)
        
        fit_btn = ttk.Button(toolbar, text="Fit", command=self.fit_to_window)
        fit_btn.pack(side=tk.LEFT, padx=2)
        
        # Separator
        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)
        
        # Reset button
        reset_btn = ttk.Button(toolbar, text="Reset Image", command=self.reset_image)
        reset_btn.pack(side=tk.LEFT, padx=2)
        
        clear_btn = ttk.Button(toolbar, text="Clear All", command=self.clear_all)
        clear_btn.pack(side=tk.LEFT, padx=2)
    
    def create_canvas(self):
        """Create canvas for image display"""
        canvas_frame = ttk.Frame(self.root)
        canvas_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        
        self.canvas = tk.Canvas(canvas_frame, bg='gray50', highlightthickness=0)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # Scrollbars
        v_scrollbar = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL, command=self.canvas.yview)
        v_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        h_scrollbar = ttk.Scrollbar(self.root, orient=tk.HORIZONTAL, command=self.canvas.xview)
        h_scrollbar.pack(side=tk.BOTTOM, fill=tk.X)
        
        self.canvas.config(xscrollcommand=h_scrollbar.set, yscrollcommand=v_scrollbar.set)
    
    def create_statusbar(self):
        """Create status bar"""
        self.statusbar = ttk.Label(self.root, text="Ready - Open a PGM file to start", 
                                   relief=tk.SUNKEN, anchor=tk.W)
        self.statusbar.pack(side=tk.BOTTOM, fill=tk.X)
    
    def bind_events(self):
        """Bind keyboard and mouse events"""
        # Keyboard shortcuts
        self.root.bind('<Control-o>', lambda e: self.open_image())
        self.root.bind('<Control-s>', lambda e: self.save_image())
        self.root.bind('<Control-q>', lambda e: self.root.quit())
        self.root.bind('<Control-r>', lambda e: self.reset_image())
        self.root.bind('<Control-x>', lambda e: self.clear_all())
        self.root.bind('<Control-0>', lambda e: self.fit_to_window())
        self.root.bind('<plus>', lambda e: self.zoom_in())
        self.root.bind('<equal>', lambda e: self.zoom_in())
        self.root.bind('<minus>', lambda e: self.zoom_out())
        
        # Mouse events
        self.canvas.bind('<ButtonPress-1>', self.on_mouse_press)
        self.canvas.bind('<B1-Motion>', self.on_mouse_drag)
        self.canvas.bind('<ButtonRelease-1>', self.on_mouse_release)
        self.canvas.bind('<MouseWheel>', self.on_mouse_wheel)
        self.canvas.bind('<Button-4>', lambda e: self.zoom_in())
        self.canvas.bind('<Button-5>', lambda e: self.zoom_out())
        
        # Window resize
        self.root.bind('<Configure>', self.on_resize)
    
    def open_image(self):
        """Open a PGM file"""
        file_path = filedialog.askopenfilename(
            title="Open PGM Image",
            filetypes=[("PGM files", "*.pgm *.Pgm *.PGM"), 
                      ("All files", "*.*")]
        )
        
        if not file_path:
            return
        
        try:
            # Load image using PIL
            self.image = Image.open(file_path)
            
            # Convert to grayscale if needed
            if self.image.mode != 'L':
                self.image = self.image.convert('L')
            
            # Store original and working copy
            self.original_image = self.image.copy()
            self.image_array = np.array(self.image)
            
            # Update display
            self.fit_to_window()
            
            # Update status
            filename = file_path.split('/')[-1]
            self.statusbar.config(text=f"Loaded: {filename} | Size: {self.image.width}x{self.image.height}")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to open image:\n{str(e)}")
    
    def save_image(self):
        """Save the current image"""
        if self.image is None:
            messagebox.showwarning("Warning", "No image to save!")
            return
        
        file_path = filedialog.asksaveasfilename(
            title="Save PGM Image",
            defaultextension=".pgm",
            filetypes=[("PGM files", "*.pgm"), ("All files", "*.*")]
        )
        
        if not file_path:
            return
        
        try:
            # Update image from array
            self.image = Image.fromarray(self.image_array.astype(np.uint8), mode='L')
            self.image.save(file_path, format='PPM')
            
            self.statusbar.config(text=f"Saved: {file_path.split('/')[-1]}")
            messagebox.showinfo("Success", "Image saved successfully!")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save image:\n{str(e)}")
    
    def reset_image(self):
        """Reset to original image"""
        if self.original_image is None:
            return
        
        self.image = self.original_image.copy()
        self.image_array = np.array(self.image)
        self.display_image()
        self.statusbar.config(text="Image reset to original")
    
    def clear_all(self):
        """Clear all pixels (set to black)"""
        if self.image_array is None:
            return
        
        if messagebox.askyesno("Confirm", "Are you sure you want to clear all pixels?"):
            self.image_array[:] = 0
            self.display_image()
            self.statusbar.config(text="All pixels cleared")
    
    def display_image(self):
        """Display the current image on canvas"""
        if self.image is None:
            return
        
        # Apply zoom
        zoomed_width = int(self.image.width * self.zoom_level)
        zoomed_height = int(self.image.height * self.zoom_level)
        
        zoomed_image = self.image.resize((zoomed_width, zoomed_height), Image.Resampling.NEAREST)
        self.photo_image = ImageTk.PhotoImage(zoomed_image)
        
        # Clear canvas and display image
        self.canvas.delete("all")
        self.canvas.create_image(self.pan_x, self.pan_y, anchor=tk.NW, image=self.photo_image)
        
        # Configure scroll region
        self.canvas.config(scrollregion=(0, 0, zoomed_width, zoomed_height))
    
    def zoom_in(self):
        """Zoom in"""
        if self.image is None:
            return
        self.zoom_level = min(self.zoom_level * 1.2, 10.0)
        self.display_image()
        self.update_zoom_status()
    
    def zoom_out(self):
        """Zoom out"""
        if self.image is None:
            return
        self.zoom_level = max(self.zoom_level / 1.2, 0.1)
        self.display_image()
        self.update_zoom_status()
    
    def fit_to_window(self):
        """Fit image to window"""
        if self.image is None:
            return
        
        # Get canvas size
        canvas_width = self.canvas.winfo_width()
        canvas_height = self.canvas.winfo_height()
        
        if canvas_width <= 1 or canvas_height <= 1:
            return
        
        # Calculate zoom to fit
        width_ratio = canvas_width / self.image.width
        height_ratio = canvas_height / self.image.height
        self.zoom_level = min(width_ratio, height_ratio) * 0.95
        
        # Reset pan
        self.pan_x = 0
        self.pan_y = 0
        
        self.display_image()
        self.update_zoom_status()
    
    def update_zoom_status(self):
        """Update zoom level in status bar"""
        self.statusbar.config(text=f"Zoom: {self.zoom_level:.1%}")
    
    def update_brush_size(self, value):
        """Update brush size"""
        self.brush_size = int(float(value))
        self.brush_size_label.config(text=str(self.brush_size))
    
    def get_image_coordinates(self, canvas_x, canvas_y):
        """Convert canvas coordinates to image coordinates"""
        img_x = int((canvas_x - self.pan_x) / self.zoom_level)
        img_y = int((canvas_y - self.pan_y) / self.zoom_level)
        return img_x, img_y
    
    def erase_pixels(self, canvas_x, canvas_y):
        """Erase pixels at the given position"""
        if self.image_array is None:
            return
        
        img_x, img_y = self.get_image_coordinates(canvas_x, canvas_y)
        
        # Check bounds
        if img_x < 0 or img_x >= self.image_array.shape[1]:
            return
        if img_y < 0 or img_y >= self.image_array.shape[0]:
            return
        
        # Calculate brush area
        half_brush = self.brush_size // 2
        y_min = max(0, img_y - half_brush)
        y_max = min(self.image_array.shape[0], img_y + half_brush + 1)
        x_min = max(0, img_x - half_brush)
        x_max = min(self.image_array.shape[1], img_x + half_brush + 1)
        
        # Erase pixels (set to 0/black)
        self.image_array[y_min:y_max, x_min:x_max] = 0
        
        # Update display
        self.image = Image.fromarray(self.image_array.astype(np.uint8), mode='L')
        self.display_image()
    
    def on_mouse_press(self, event):
        """Handle mouse press"""
        if self.image is None:
            return
        
        if self.erase_var.get():
            self.is_erasing = True
            self.erase_pixels(event.x, event.y)
        else:
            self.is_dragging = True
            self.last_x = event.x
            self.last_y = event.y
    
    def on_mouse_drag(self, event):
        """Handle mouse drag"""
        if self.image is None:
            return
        
        if self.is_erasing:
            self.erase_pixels(event.x, event.y)
        elif self.is_dragging:
            # Pan the view
            dx = event.x - self.last_x
            dy = event.y - self.last_y
            self.pan_x += dx
            self.pan_y += dy
            self.last_x = event.x
            self.last_y = event.y
            self.display_image()
    
    def on_mouse_release(self, event):
        """Handle mouse release"""
        self.is_erasing = False
        self.is_dragging = False
    
    def on_mouse_wheel(self, event):
        """Handle mouse wheel for zooming"""
        if self.image is None:
            return
        
        if event.delta > 0:
            self.zoom_in()
        else:
            self.zoom_out()
    
    def on_resize(self, event):
        """Handle window resize"""
        if event.widget == self.root and self.image is not None:
            # Delay redraw to avoid excessive updates
            self.root.after(100, self.fit_to_window)
    
    def show_about(self):
        """Show about dialog"""
        about_text = """PGM Image Viewer
        
A simple viewer for PGM (Portable GrayMap) images with basic pixel erasing functionality.

Features:
- Open PGM files (P2 ASCII and P5 binary)
- Erase pixels with adjustable brush size
- Zoom in/out and pan
- Save modified images

Keyboard Shortcuts:
- Ctrl+O: Open image
- Ctrl+S: Save image
- Ctrl+R: Reset image
- Ctrl+X: Clear all
- +/-: Zoom in/out
- Ctrl+0: Fit to window

Created with Python, Tkinter, and PIL/Pillow."""
        
        messagebox.showinfo("About PGM Viewer", about_text)


def main():
    root = tk.Tk()
    root.geometry("800x600")
    
    app = PGMViewer(root)
    
    root.mainloop()


if __name__ == "__main__":
    main()
