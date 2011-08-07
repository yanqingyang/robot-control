#include "robot-control/CGtk.h"

static void close_window(){gtk_main_quit();}

CGtk::CGtk(CQPed *Q){ 
    running = 0; 
    selected_leg = 0;
    gtk_init(NULL,NULL);
    //build gui elements
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window),"Robot_control");
    vbox_main = gtk_vbox_new(FALSE,2);
    hbox_main = gtk_hbox_new(FALSE,2);
    vbox_left = gtk_vbox_new(FALSE,2);
    gtk_widget_set_size_request(vbox_left, 200, -1);
    hbox_button = gtk_hbox_new(FALSE,2);
    vbox_right = gtk_vbox_new(TRUE,2);
    gtk_widget_set_size_request(vbox_right, 100, -1);
    vbox_mid = gtk_vbox_new(TRUE,2);
    gtk_box_pack_start(GTK_BOX(vbox_main), hbox_main, FALSE, TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox_main), vbox_left,FALSE, TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox_main), vbox_mid,TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(hbox_main), vbox_right,TRUE, TRUE,2);

    uint8_t i;
    char text[100];
    //left
    button_connect = gtk_button_new();
    show_disconnected();
    gtk_widget_set_size_request(button_connect, 32,32);
    button_leg = gtk_button_new();
    show_right();
    gtk_widget_set_size_request(button_leg, 32,32);
    gtk_box_pack_start(GTK_BOX(vbox_left), hbox_button,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox_button),button_connect,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox_button),button_leg,FALSE,FALSE,0);
    for(i=0;i<SERVOS;i++){
        sprintf(text, GUI_SERVO_LABEL_FORMAT, i,0.0,0);
        servo_label[i] = gtk_label_new(NULL); //gtk_check_button_new_with_label(text);
        gtk_label_set_markup(GTK_LABEL(servo_label[i]), text);
        gtk_box_pack_start(GTK_BOX(vbox_left), servo_label[i],TRUE,TRUE,0);
    }
    //mid
    da = gtk_drawing_area_new();
    gtk_widget_set_size_request(da, 300,-1);
    gtk_box_pack_start(GTK_BOX(vbox_mid),da,TRUE,TRUE,0);
    //right
    for(i=0;i<QP_LEGS;i++){
        sprintf(text, "X%d Y%d Z%d",i,i,i);
        position_label[i] = gtk_label_new(text);
        gtk_box_pack_start(GTK_BOX(vbox_right), position_label[i], TRUE, TRUE, 0);
    }
    adc_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox_right), adc_label, TRUE, TRUE, 0);
    //gamepad drawing
    gamepadDrawing = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox_main), gamepadDrawing, TRUE, TRUE,0);
    gtk_widget_set_size_request(gamepadDrawing, -1, 150);
    //add main container to window
    gtk_container_add(GTK_CONTAINER(window), vbox_main);
    //connect signals
    g_signal_connect(window, "destroy", G_CALLBACK(close_window),NULL);
    g_signal_connect(window, "key_press_event", G_CALLBACK(key_press_callback), (gpointer)this);
    g_signal_connect(button_connect, "clicked", G_CALLBACK(connect_clicked_cb), (gpointer)this);
    g_signal_connect(button_leg, "clicked", G_CALLBACK(controller_clicked_cb), (gpointer)this);
    connect_timeout();
    g_signal_connect(G_OBJECT(da), "expose_event", G_CALLBACK(paint), (gpointer)this);
    g_signal_connect(G_OBJECT(gamepadDrawing),
         "expose_event", G_CALLBACK(paintGP), (gpointer)this);    
    gtk_widget_show_all(window);
    qp = Q;
}

void CGtk::show_connected(){
    GtkWidget *img;
    img = gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button_connect),img);
}
void CGtk::show_disconnected(){
    GtkWidget *img;
    img = gtk_image_new_from_stock(GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button_connect),img);
}

void CGtk::show_left(){
    GtkWidget *img;
    img = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button_leg),img);
}

void CGtk::show_right(){
    GtkWidget *img;
    img = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button_leg),img);
}

void CGtk::updateADC(){
    char text[100];
    sprintf(text, GUI_ADC_LABEL_FORMAT,
    qp->adc[0],
    qp->filterX.x, 
    qp->adc[1],
    qp->filterY.x);
    gtk_label_set_markup(GTK_LABEL(adc_label), text);
}

void CGtk::updateGamePadDrawing(){
    paintGP(gamepadDrawing, NULL, this);
}

void CGtk::updateServoData(){
    qp->readFromDev();
    uint8_t i;
    char text[100];
    for(i=0;i<SERVOS;i++){
        sprintf(text, GUI_SERVO_LABEL_FORMAT, i,
            qp->getAngle(i),
            qp->getPW(i)
        );
        //gtk_button_set_label(GTK_BUTTON(servo_label[i]),text);
        gtk_label_set_markup(GTK_LABEL(servo_label[i]),text);
    }
}

void CGtk::connect_timeout(){
    timeoutID = g_timeout_add_full(
        G_PRIORITY_DEFAULT,
        GUI_TIMEOUT,
        timeout1, 
        (gpointer)this, 
        timeout_disconnected);
}

void CGtk::run(){
    running=1;
    gtk_main();
}

void CGtk::updatePositions(){
    uint8_t i;
    char text[50];
    for(i=0;i<QP_LEGS;i++){
        sprintf(text, GUI_POSITION_LABEL_FORMAT,
            qp->getRelativeServoX(i, LEG_ENDPOINT),
            qp->getRelativeServoY(i, LEG_ENDPOINT),
            qp->getRelativeServoZ(i, LEG_ENDPOINT)
        );
        gtk_label_set_text(GTK_LABEL(position_label[i]),text);
    }
}

 

static gboolean key_press_callback(GtkWidget* widget, GdkEvent *event, gpointer data){
    guint key = ((GdkEventKey*)event)->keyval;
    CGtk* gui = ((CGtk*)data);
    switch(key){
    case 'q':
        gui->running = 0;
        break;
    case '`':
        gui->qp->reset();
        gui->qp->sendToDev();
        break;
    case 'w':
        if(gui->qp->changeAllLegs(0,-GUI_KEYBOARD_SPEED,0)==0)
        gui->qp->sendToDev();
        break;
    case 's':
        if(gui->qp->changeAllLegs(0,GUI_KEYBOARD_SPEED,0)==0)
        gui->qp->sendToDev();
        break;
    case 'a':
        if(gui->qp->changeAllLegs(GUI_KEYBOARD_SPEED,0,0)==0)
        gui->qp->sendToDev();
        break;
    case 'd':
        if(gui->qp->changeAllLegs(-GUI_KEYBOARD_SPEED,0,0)==0)
        gui->qp->sendToDev();
        break;
    case 'W':
        if(gui->qp->changeSingleLeg(gui->selected_leg, 0, -GUI_KEYBOARD_SPEED,0)==0)
        gui->qp->sendToDev();
        break;
    case 'S':
        if(gui->qp->changeSingleLeg(gui->selected_leg, 0, GUI_KEYBOARD_SPEED,0)==0)
        gui->qp->sendToDev();
        break;
    case 'A':
        if(gui->qp->changeSingleLeg(gui->selected_leg, GUI_KEYBOARD_SPEED,0,0)==0)
        gui->qp->sendToDev();
        break;
    case 'D':
        if(gui->qp->changeSingleLeg(gui->selected_leg, -GUI_KEYBOARD_SPEED,0,0)==0)
        gui->qp->sendToDev();
        break;
    case 'f':
        if(gui->qp->changeMainBodyAngle(0,GUI_KEYBOARD_SPEED/10,0)==0)
        gui->qp->sendToDev();
        break;
    case 'h':
        if(gui->qp->changeMainBodyAngle(0,-GUI_KEYBOARD_SPEED/10,0)==0)
        gui->qp->sendToDev();
        break;
    case 't':
        if(gui->qp->changeMainBodyAngle(0,0,GUI_KEYBOARD_SPEED/10)==0)
        gui->qp->sendToDev();
        break;
    case 'g':
        if(gui->qp->changeMainBodyAngle(0,0,-GUI_KEYBOARD_SPEED/10)==0)
        gui->qp->sendToDev();
        break;
    case 'z':
        if(gui->qp->changeAllLegs(0,0,-GUI_KEYBOARD_SPEED)==0)
        gui->qp->sendToDev();
        break;
    case 'x':
        if(gui->qp->changeAllLegs(0,0,GUI_KEYBOARD_SPEED)==0)
        gui->qp->sendToDev();
        break;
    }
    usleep(10000);//allow device to transmit before next command;
    gui->updateServoData();
    gui->updatePositions();
    paint(gui->da, NULL, gui);
    return TRUE;
}


static void connect_clicked_cb(GtkButton *button, gpointer data){
    CGtk* gui = ((CGtk*)data);
    gui->qp->reset();
    gui->qp->sendToDev();
    timeout1(data);
}
static void controller_clicked_cb(GtkButton *button, gpointer data){
    CGtk* gui = ((CGtk*)data);
    if(gui->selected_leg == 0){
        gui->selected_leg = 1;
        gui->show_left();
    }else{
        gui->selected_leg = 0;
        gui->show_right();
    }
}


static gboolean timeout1(gpointer data){
    CGtk* gui = ((CGtk*)data);
    if(gui->running == 0) return FALSE;
    gui->qp->getUsbData();
    gui->qp->fillPSController();
//    gui->qp->fillADC();
//    gui->updateADC();
    gui->updateGamePadDrawing();
    if(gui->qp->moveByStick()){
        gui->qp->sendToDev();
    //usleep(10000);//allow device to transmit before next command;
    //gui->updateServoData();

    }
    gui->updatePositions();    
        paint(gui->da, NULL, gui);
    
    if(gui->qp->getConnected()>1) gui->show_connected();
    else gui->show_disconnected();
    return TRUE;
}

static void timeout_disconnected(gpointer data){
    g_print("disconnected timeout\n");//placeholder
}

void drawTriangle(cairo_t *cr, double x, double y, double size, uint8_t turn){
    double mirror;
    if(turn % 2){ // 1 and 3
        mirror = pow(-1.0,turn/2);
        cairo_move_to(cr, x + mirror * size/2, y);
        cairo_rel_line_to(cr, -mirror*size, size/2);
        cairo_rel_line_to(cr, 0, -size);        
    }else{ //0 and 2
        mirror =  pow(-1.0, turn/2);
        cairo_move_to(cr, x, y - mirror *size/2);
        cairo_rel_line_to(cr, size/2, mirror * size);
        cairo_rel_line_to(cr, -size,0);
    }
    cairo_close_path(cr);
    cairo_stroke(cr);
}


#define BG_COLOR 1
#define DPADSPACING 35 //also used for shapes
#define STICKSIZE  60
//draw shapes for evey button, vary the color depending on the button state
static void paintGP(GtkWidget *widget, GdkEventExpose *eev, gpointer data){
    CGtk* gui = ((CGtk*)data);
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    cairo_t *cr;
    cr = gdk_cairo_create(widget->window);
    cairo_set_source_rgb(cr,BG_COLOR,BG_COLOR,BG_COLOR);
    cairo_paint(cr);
    const double baseX = alloc.width/6*3;
    const double baseY = alloc.height/3;
//DPAD
    const double dpadCenterX = alloc.width/6*1;
    const double dpadCenterY = alloc.height * 1/3;
    //up button
    if(gui->qp->pscon.getSSDpad(UP)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    drawTriangle(cr, dpadCenterX, dpadCenterY - DPADSPACING, 20,0);
    //down button
    if(gui->qp->pscon.getSSDpad(DOWN)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    drawTriangle(cr, dpadCenterX, dpadCenterY + DPADSPACING, 20,2);
    //right button
    if(gui->qp->pscon.getSSDpad(RIGHT)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    drawTriangle(cr, dpadCenterX+DPADSPACING, dpadCenterY, 20,1);
    //left button
    if(gui->qp->pscon.getSSDpad(LEFT)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    drawTriangle(cr, dpadCenterX-DPADSPACING, dpadCenterY, 20,3);
//START SELECT
    //start
    if(gui->qp->pscon.getSSDpad(START)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    cairo_rectangle(cr, baseX+DPADSPACING/2, baseY,30, 15);
    cairo_stroke(cr);
    //select
    if(gui->qp->pscon.getSSDpad(SELECT)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    cairo_rectangle(cr, baseX-30-DPADSPACING/2, baseY,30, 15);
    cairo_stroke(cr);
//shapes
    const double shapesCenterX = alloc.width/6*5;
    const double shapesCenterY = alloc.height * 1/3;
    //triangle
    if(gui->qp->pscon.getShoulderShapes(TRIANGLE)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    drawTriangle(cr, shapesCenterX, shapesCenterY - DPADSPACING, 20,0);
    //cross
    if(gui->qp->pscon.getShoulderShapes(CROSS)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    cairo_move_to(cr, shapesCenterX+10, shapesCenterY + DPADSPACING-10);
    cairo_rel_line_to(cr, -20, +20);
    cairo_rel_move_to(cr, 20,0);
    cairo_rel_line_to(cr, -20, -20);
    cairo_stroke(cr);
    //square
    if(gui->qp->pscon.getShoulderShapes(SQUARE)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    cairo_rectangle(cr, shapesCenterX-DPADSPACING-10,shapesCenterY-10,20,20);
    cairo_stroke(cr);
    //circle
    if(gui->qp->pscon.getShoulderShapes(CIRCLE)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    cairo_arc(cr, shapesCenterX + DPADSPACING, shapesCenterY, 10, 0, -2*PI-0.0001);
    cairo_stroke(cr);
//left stick
    const double leftCenterX = (dpadCenterX+baseX)/2;
    const double leftCenterY = alloc.height/3*2;
    //boundary circle
    cairo_set_source_rgb(cr, 0,0,0);
    cairo_move_to(cr, leftCenterX,leftCenterY-STICKSIZE/2);
    cairo_rel_line_to(cr, 0, STICKSIZE);
    cairo_move_to(cr, leftCenterX-STICKSIZE/2, leftCenterY);
    cairo_rel_line_to(cr, STICKSIZE, 0);
    cairo_arc(cr, leftCenterX, leftCenterY, STICKSIZE/2, 0, -2*PI-0.0001);
    cairo_stroke(cr);
    //marker
    cairo_set_source_rgb(cr, 1,0,0);
    double dx = ((double)gui->qp->pscon.getLx())/140*STICKSIZE/2;
    double dy = ((double)gui->qp->pscon.getLy())/140*STICKSIZE/2;
    cairo_arc(cr, leftCenterX +dx, leftCenterY + dy,STICKSIZE/10, 0, -2*PI-0.0001);
    cairo_stroke(cr);
//right stick
    const double rightCenterX = (shapesCenterX + baseX)/2;
    const double rightCenterY = alloc.height/3*2;
    //boundary circle
    cairo_set_source_rgb(cr, 0,0,0);
    cairo_move_to(cr, rightCenterX,rightCenterY-STICKSIZE/2);
    cairo_rel_line_to(cr, 0, STICKSIZE);
    cairo_move_to(cr, rightCenterX-STICKSIZE/2, rightCenterY);
    cairo_rel_line_to(cr, STICKSIZE, 0);
    cairo_arc(cr, rightCenterX, rightCenterY, STICKSIZE/2, 0, -2*PI-0.0001);
    cairo_stroke(cr);
    //marker
    cairo_set_source_rgb(cr, 1,0,0);
    dx = ((double)gui->qp->pscon.getRx())/140*STICKSIZE/2;
    dy = ((double)gui->qp->pscon.getRy())/140*STICKSIZE/2;
    cairo_arc(cr, rightCenterX +dx, leftCenterY + dy,STICKSIZE/10, 0, -2*PI-0.0001);
    cairo_stroke(cr);
//shoulder buttons
    //L1
    if(gui->qp->pscon.getShoulderShapes(L1)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    cairo_rectangle(cr, 0, 0, 30, 15);
    cairo_stroke(cr);
    //L2
    if(gui->qp->pscon.getShoulderShapes(L2)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    cairo_rectangle(cr, 0, 30, 30, 15);
    cairo_stroke(cr);
    //R1
    if(gui->qp->pscon.getShoulderShapes(R1)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    cairo_rectangle(cr, alloc.width-30, 0, 30, 15);
    cairo_stroke(cr);
    //R2
    if(gui->qp->pscon.getShoulderShapes(R2)) cairo_set_source_rgb(cr, 1,0,0);
    else cairo_set_source_rgb(cr, 0,0,0);
    cairo_rectangle(cr, alloc.width-30, 30, 30, 15);
    cairo_stroke(cr);
                    
//grid
//    cairo_move_to(cr, alloc.width/6*1, 0);
//    cairo_rel_line_to(cr, 0, alloc.height);
//    cairo_move_to(cr, alloc.width/6*2, 0);
//    cairo_rel_line_to(cr, 0, alloc.height);
//    cairo_move_to(cr, alloc.width/6*3, 0);
//    cairo_rel_line_to(cr, 0, alloc.height);
//    cairo_move_to(cr, alloc.width/6*4, 0);
//    cairo_rel_line_to(cr, 0, alloc.height);
//    cairo_move_to(cr, alloc.width/6*5, 0);
//    cairo_rel_line_to(cr, 0, alloc.height);
//    cairo_stroke(cr);    
        cairo_destroy(cr);    

}

//no rotation support, deprecated
void drawLeg(cairo_t *cr, gpointer data,  uint8_t leg, double  startX,double startY){
    CGtk* gui = ((CGtk*)data);    
    cairo_set_source_rgb(cr, 0,0.8, 0);
    double x = startX;
    double y = startY;
    //endpoint -> servo 2
    cairo_rectangle(cr, x-5, y-5, 10, 10);
    cairo_move_to(cr, x,y);
    x +=( gui->qp->legs[leg]->getX(2) -gui->qp->legs[leg]->getX(3) )*GUI_DRAW_SCALE; 
    y -=( gui->qp->legs[leg]->getY(2) -gui->qp->legs[leg]->getY(3) )*GUI_DRAW_SCALE; 
    cairo_line_to(cr, x,y);
    
    cairo_rectangle(cr, x-5, y-5, 10, 10);
    cairo_move_to(cr, x,y);
    x +=( gui->qp->legs[leg]->getX(1) -gui->qp->legs[leg]->getX(2) )*GUI_DRAW_SCALE; 
    y -=( gui->qp->legs[leg]->getY(1) -gui->qp->legs[leg]->getY(2) )*GUI_DRAW_SCALE; 
    cairo_line_to(cr, x,y);

    cairo_rectangle(cr, x-5, y-5, 10, 10);
    cairo_move_to(cr, x,y);
    x +=( gui->qp->legs[leg]->getX(0) -gui->qp->legs[leg]->getX(1) )*GUI_DRAW_SCALE; 
    y -=( gui->qp->legs[leg]->getY(0) -gui->qp->legs[leg]->getY(1) )*GUI_DRAW_SCALE; 
    cairo_line_to(cr, x,y);
    cairo_rectangle(cr, x-5, y-5, 10, 10);
    
    cairo_stroke(cr);
}

void drawLeg_around_0(cairo_t *cr, gpointer data,  uint8_t leg, double  startX,double startY){
    CGtk* gui = ((CGtk*)data);    
    double x = startX;
    double y = startY;
    //0 -> servo 0
//    cairo_rectangle(cr, x-5, y-5, 10, 10);
    cairo_move_to(cr, x,y);
    x +=( gui->qp->legs[leg]->getX(0) )*GUI_DRAW_SCALE; 
    y -=( gui->qp->legs[leg]->getY(0) )*GUI_DRAW_SCALE; 
    cairo_line_to(cr, x,y);
    //servo 0 -> servo 1
    cairo_rectangle(cr, x-5, y-5, 10, 10);
    cairo_move_to(cr, x,y);
    x +=( gui->qp->legs[leg]->getX(1) -gui->qp->legs[leg]->getX(0) )*GUI_DRAW_SCALE; 
    y -=( gui->qp->legs[leg]->getY(1) -gui->qp->legs[leg]->getY(0) )*GUI_DRAW_SCALE; 
    cairo_line_to(cr, x,y);
    //servo 1 -> servo 2    
    cairo_rectangle(cr, x-5, y-5, 10, 10);
    cairo_move_to(cr, x,y);
    x +=( gui->qp->legs[leg]->getX(2) -gui->qp->legs[leg]->getX(1) )*GUI_DRAW_SCALE; 
    y -=( gui->qp->legs[leg]->getY(2) -gui->qp->legs[leg]->getY(1) )*GUI_DRAW_SCALE; 
    cairo_line_to(cr, x,y);
    //servo 2 -> endPoint
    cairo_rectangle(cr, x-5, y-5, 10, 10);
    cairo_move_to(cr, x,y);
    x +=( gui->qp->legs[leg]->getX(3) -gui->qp->legs[leg]->getX(2) )*GUI_DRAW_SCALE; 
    y -=( gui->qp->legs[leg]->getY(3) -gui->qp->legs[leg]->getY(2) )*GUI_DRAW_SCALE; 
    cairo_line_to(cr, x,y);
    cairo_rectangle(cr, x-5, y-5, 10, 10);
    
    cairo_stroke(cr);
}

void drawLineThrough(cairo_t *cr, double x1, double y1, double x2, double y2){
    const double dx = x2 - x1;
    const double dy = y2 - y1;
    const double b = y1 - dy/dx * x1;
    //assume color is set
    cairo_move_to(cr, x1, y1);
    cairo_rel_move_to(cr, -x1, dy/dx * -x1);
    cairo_rel_line_to(cr, x2 + x1, dy/dx * (x1+x2));
    cairo_stroke(cr);
}

static void paint(GtkWidget *widget, GdkEventExpose *eev, gpointer data){
    CGtk* gui = ((CGtk*)data);
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    cairo_t *cr;
    cr = gdk_cairo_create(widget->window);
    cairo_set_line_width(cr,GUI_LINEWIDTH);
    //clear area
    cairo_set_source_rgb(cr,BG_COLOR,BG_COLOR,BG_COLOR);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 1,0,0);  
    drawLeg_around_0(cr,data, 2, alloc.width/2, alloc.height/2);
    drawLeg_around_0(cr,data, 3, alloc.width/2, alloc.height/2);
    cairo_set_source_rgb(cr, 0,0, 0.9);
    drawLeg_around_0(cr,data, 0, alloc.width/2, alloc.height/2);
    drawLeg_around_0(cr,data, 1, alloc.width/2, alloc.height/2);
    
    //draw line through both endPoints
    cairo_set_source_rgb(cr,1,0,0);    
    drawLineThrough(cr,
        alloc.width/2 +  gui->qp->legs[1]->getX(3) * GUI_DRAW_SCALE,
        alloc.height/2 - gui->qp->legs[1]->getY(3) * GUI_DRAW_SCALE,
        alloc.width/2 +  gui->qp->legs[0]->getX(3) * GUI_DRAW_SCALE,
        alloc.height/2 - gui->qp->legs[0]->getY(3) * GUI_DRAW_SCALE
    );
    //line thourgh main body
    cairo_set_source_rgb(cr,0,0,0);    
    cairo_set_line_width(cr,1);    
    
    drawLineThrough(cr,
        alloc.width/2 +  gui->qp->legs[1]->getX(0) * GUI_DRAW_SCALE,
        alloc.height/2 - gui->qp->legs[1]->getY(0) * GUI_DRAW_SCALE,
        alloc.width/2 +  gui->qp->legs[0]->getX(0) * GUI_DRAW_SCALE,
        alloc.height/2 - gui->qp->legs[0]->getY(0) * GUI_DRAW_SCALE
    );
    
    cairo_destroy(cr);
    return;
}


