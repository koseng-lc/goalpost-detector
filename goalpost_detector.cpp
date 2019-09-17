#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>

using namespace cv;
using namespace std;

void segmenWarna(Mat& in,Mat& lut_gmm, Mat& hijau, Mat& putih, Mat& hitam){
    hijau = Mat::zeros(in.size(),CV_8UC1);
    putih = Mat::zeros(in.size(),CV_8UC1);
    hitam = Mat::zeros(in.size(),CV_8UC1);
    int kolom = in.cols;
    int baris = in.rows;
    int knl = in.channels();
    uchar* in_data = in.data;

    for(int i=0;i<baris;i++){
        for(int j=0;j<kolom;j++){
            int idx = (i*kolom+j)*knl;
            int B = in_data[idx+0]>>2;
            int G = in_data[idx+1]>>2;
            int R = in_data[idx+2]>>2;
            int idx_tabel = B<<12 | G<<6 | R;
            switch(lut_gmm.at<uchar > (idx_tabel)){
                case 0:in_data[idx]=0;in_data[idx+1]=255;in_data[idx+2]=0;hijau.at<uchar > (idx/knl) = 255;break;
                case 1:in_data[idx]=255;in_data[idx+1]=255;in_data[idx+2]=255;putih.at<uchar > (idx/knl) = 255;break;
                case 2:in_data[idx]=0;in_data[idx+1]=0;in_data[idx+2]=0;hitam.at<uchar > (idx/knl)=255;break;
                default:break;
            }
        }
    }
}


void cropField(Mat &input, Mat &hsv, Mat &thresh, Mat &output){    
    Mat mat_thresh = Mat::zeros(input.size(),CV_8UC1);
    Mat hvs = Mat::zeros(input.size(),CV_8UC1);
    Mat mat_HSV;

    double dT=getTickCount();
    cvtColor(input,mat_HSV,CV_BGR2HSV);    
    inRange(mat_HSV,Scalar(36,177,96),Scalar(44,255,195),mat_thresh);    
    dT=(getTickCount()-dT)/getTickFrequency();
    cout<<"TES:"<<dT<<endl;
    vector<vector<Point > > contours;

    findContours(mat_thresh,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);

    vector<Point > titik_kontur;

    for(vector<vector<Point > >::const_iterator it=contours.begin();it!=contours.end();it++){
        if(contourArea(Mat(*it))>1000)
            titik_kontur.insert(titik_kontur.end(),it->begin(),it->end());
    }

    if(titik_kontur.size()){
        vector<Point > titik_hull;
        vector<vector<Point > > contours2;
        convexHull(titik_kontur,titik_hull);
        contours2.push_back(titik_hull);
        drawContours(hvs,contours2,0,Scalar::all(255),CV_FILLED);
        divide(255,hvs,hvs);
    }
    mat_thresh.copyTo(thresh);
    mat_HSV.copyTo(hsv);
    hvs.copyTo(output);
}

Point* cropLuar(Mat &thresh_hijau,Mat &kontur_lapang, int &sz){
    Point* batas_lapang;
    Mat hvs=Mat::zeros(thresh_hijau.size(),CV_8UC1);
    vector<Point > titik_kontur;
    vector<vector<Point > > contours;

    findContours(thresh_hijau,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);

    for(size_t i=0;i<contours.size();i++){
        if((contourArea(contours[i])>800)){
            titik_kontur.insert(titik_kontur.end(),contours[i].begin(),contours[i].end());
        }
    }
    int radius_node=5;
    if(titik_kontur.size()){
        vector<Point > titik_hull;
        vector<vector<Point > > contours2;
        convexHull(titik_kontur,titik_hull);
        contours2.push_back(titik_hull);
        Rect r = boundingRect(titik_hull);

        batas_lapang = new Point[r.width];
        //Kontur penuh
        drawContours(hvs,contours2,0,Scalar(255),CV_FILLED);
        //cvtColor(hvs,hvs,CV_BGR2GRAY);
        uchar *hvs_data = hvs.data;
        int h=0;
        for(int i=radius_node;i<hvs.cols-radius_node;i++){
            batas_lapang[h]=Point(i,thresh_hijau.rows-1);
            for(int j=radius_node;j<hvs.rows-radius_node;j++){
                int idx = j*hvs.cols + i;
                if(hvs_data[idx]){
                    batas_lapang[h]=Point(i,j);
                    h++;
                    break;
                }
            }
        }
        sz=h;
    }else{

        batas_lapang = new Point[1];
        batas_lapang[0]=Point(0,0);

    }
    hvs.copyTo(kontur_lapang);
    return batas_lapang;
}

int cek3X3(uchar* pth,int idx,int jml_kolom){
    int cek=0;
    for(int i=-jml_kolom;i<=jml_kolom;i+=jml_kolom){
        for(int j=-1;j<=1;j++){
            int tmp = idx + i + j;
            if(pth[tmp])cek++;
        }
    }
    return cek;
}

int main(){
    //system("v4l2-ctl --device=1 --set-ctrl exposure_auto_priority=0,exposure_auto=1,exposure_absolute=300,brightness=100,contrast=150,saturation=200,gain=128,focus_auto=0,white_balance_temperature_auto=0,white_balance_temperature=5500");
    //system("v4l2-ctl --device=1 --set-ctrl exposure_auto_priority=0,exposure_auto=1,exposure_absolute=300,brightness=100,contrast=150,saturation=200,gain=128,focus_auto=0,white_balance_temperature_auto=0,white_balance_temperature=5500");
    //system("v4l2-ctl --device=1 --set-ctrl exposure_auto_priority=0,exposure_auto=3,exposure_absolute=300,brightness=100,contrast=150,saturation=200.focus_auto=0,white_balance_temperature_auto=0,white_balance_temperature=5500");

    VideoCapture vc;
    vc.open("/media/koseng/563C3F913C3F6ADF/ada gawang.avi");
    //vc.open("/media/koseng/563C3F913C3F6ADF/ada tiang.avi");
    //vc.open("/media/koseng/563C3F913C3F6ADF/Photos/ikut/CIT Brains Robocup 2016 Qualification Video Kidsize Humanoid League.mp4");
    //vc.open(1);
    if(!vc.isOpened()){
        cout<<"VIDEO GAK KELOAD"<<endl;
        return -1;
    }

    //=======================================================================================
    Mat lut_gmm;
    FileStorage fs("/media/koseng/563C3F913C3F6ADF/bola/tabel_warna.xml",FileStorage::READ);
    fs["Tabel_Warna"] >> lut_gmm;
    fs.release();
    //=======================================================================================    

    Mat temp;
    vc >> temp;    

    const int ambang_gradien=127;
    const int panjang_minimal=110;

    Mat img;

    Mat img2;

    Mat hsv;
    Mat hijau;
    Mat putih;
    Mat hitam;

    Mat kontur_lapangan;

    Mat transdy;
    Mat dy(temp.size(),CV_8UC1),dx(temp.size(),CV_8UC1);    

    temp.release();



    while(1){

        vc >> img;
        //flip(img,img,-1);
        Mat gray;
        cvtColor(img,gray,CV_BGR2GRAY);
        //threshold(gray,gray,ambang_gradien,255.0,THRESH_OTSU);
        int jml_kolom = img.cols;

        cvtColor(img,hsv,CV_BGR2HSV);
        segmenWarna(hsv,lut_gmm,hijau,putih,hitam);

        int sz=0;

        Point* batas_lapang(cropLuar(hijau,kontur_lapangan,sz));

        //target cari lebar terbesar

        vector<Point > kandidat_tiang;
        Point kandidat(-1,-1);                

        uchar *tp_data = putih.data;

        img.copyTo(img2);

        if(!sz)continue;
        for(int i=0;i<sz;i++){
            int idx = batas_lapang[i].x + batas_lapang[i].y*jml_kolom;
            if(tp_data[idx]&&kandidat.x==-1){
                kandidat = batas_lapang[i];
            }else if(!tp_data[idx]&&kandidat.x!=-1){
                kandidat.x = (batas_lapang[i].x + kandidat.x)/2;
                kandidat.y = (batas_lapang[i].y + kandidat.y)/2;
                kandidat_tiang.push_back(kandidat);
                kandidat = Point(-1,-1);
            }
        }

        std::cout << "TEST" << std::endl;

        //tinggi atas
        double dT=getTickCount();
        Sobel(gray,dy,CV_8U,0,1,3);
        transpose(dy,transdy);
        int maks=0;
        int maks2=0;
        int indeks=-1,indeks2=-1;
        for(size_t i=0;i<kandidat_tiang.size();i++){
            //uchar *transdy_ptr = transdy.data + transdy.step*kandidat_tiang[i].x;
            int pos_x = kandidat_tiang[i].x;
            int panjang=0;
            for(int j=kandidat_tiang[i].y;j>1;j--){
                int idx=pos_x + j*jml_kolom;
                if(tp_data[idx])panjang++;
                if(j==0||tp_data[kandidat_tiang[i].x + j*jml_kolom]==0){
                    if(cek3X3(tp_data,idx,jml_kolom)&&j!=0){
                        pos_x--;
                        continue;
                    }
                    if(panjang>maks&&panjang>panjang_minimal){maks=panjang;indeks=i;}
                    if(panjang>maks2&&panjang<maks&&panjang>panjang_minimal){maks2=panjang;indeks2=i;}
                    break;
                }
            }
        }
        //tinggi bawah
        int bawah=0,bawah2=0;
        if(indeks!=-1){
            //uchar *transdy_data = transdy.data;
            int panjang=0;
            for(int i=kandidat_tiang[indeks].y;i<transdy.cols;i++){
                int idx=kandidat_tiang[indeks].x + i*jml_kolom;
                panjang++;
                if(tp_data[idx]){
                    bawah=panjang;
                    break;
                }
            }
            panjang=0;
            if(indeks2!=-1){
                for(int i=kandidat_tiang[indeks2].y;i<transdy.cols;i++){
                    int idx=kandidat_tiang[indeks2].x + i*jml_kolom;
                    panjang++;
                    if(tp_data[idx]){
                        bawah2=panjang;
                        break;
                    }
                }
            }
        }

        Sobel(gray,dx,CV_8U,1,0,3);
        if(indeks==-1){
            putText(img2,"GAWANG TIDAK TERDETEKSI",Point(20,20),CV_FONT_HERSHEY_SIMPLEX,1,Scalar(0,0,255),2);
        }else if(indeks2==-1){
            int hitung_putih=0;
            uchar *dx_ptr = dx.data + dx.step*(kandidat_tiang[indeks].y-maks);
            for(int i=kandidat_tiang[indeks].x;i<dx.cols&&i<kandidat_tiang[indeks].x+panjang_minimal;i++){
                if(dx_ptr[i]<ambang_gradien)hitung_putih++;
                else break;
            }
            if(hitung_putih>20)putText(img2,"KIRI",Point(20,20),CV_FONT_HERSHEY_SIMPLEX,1,Scalar(0,0,255),2);
            else putText(img2,"KANAN",Point(20,20),CV_FONT_HERSHEY_SIMPLEX,1,Scalar(0,0,255),2);
            line(img2,Point(kandidat_tiang[indeks].x,kandidat_tiang[indeks].y-maks),Point(kandidat_tiang[indeks].x,kandidat_tiang[indeks].y+bawah),Scalar(0,0,255),3);
            circle(img2,kandidat_tiang[indeks],3,Scalar(0,255,0),CV_FILLED);

        }else{
            line(img2,Point(kandidat_tiang[indeks].x,kandidat_tiang[indeks].y-maks),Point(kandidat_tiang[indeks].x,kandidat_tiang[indeks].y+bawah),Scalar(0,0,255),3);
            line(img2,Point(kandidat_tiang[indeks2].x,kandidat_tiang[indeks2].y-maks2),Point(kandidat_tiang[indeks2].x,kandidat_tiang[indeks2].y+bawah2),Scalar(0,0,255),3);
            circle(img2,kandidat_tiang[indeks],3,Scalar(0,255,0),CV_FILLED);
            circle(img2,kandidat_tiang[indeks2],3,Scalar(0,255,0),CV_FILLED);
        }
        //namedWindow("ngetes",CV_WINDOW_NORMAL);        
        dT = (getTickCount()-dT)/getTickFrequency();

        cout<<dT<<endl;

        imshow("ngetes",img2);
        //imshow("thr",thresh);

        imshow("THP",putih);
        bitwise_and(putih,kontur_lapangan,putih);
        imshow("CROPPTH",putih);
        cvtColor(kontur_lapangan,kontur_lapangan,CV_GRAY2BGR);
        bitwise_and(img2,kontur_lapangan,img2);
        imshow("EA",img2);
        //imshow("HSV",mHSV);
        imshow("DY",dy);
        imshow("DX",dx);

        int wk=waitKey(1);
        if(wk==27)break;
    }
}

 /*TIMELINE

 for(size_t i=0;i<contours.size();i++){
    if((contourArea(contours[i])>1000))titik_kontur.insert(titik_kontur.end(),contours[i].begin(),contours[i].end());
 }

 const int kernel_htl = temp.cols/20;

 //seleksi mistar(garis horizontal)
 erode(thresh_putih,mistar,getStructuringElement(MORPH_RECT,Size(kernel_htl,1)));

 //seleksi tiang(garis vertical)
 erode(thresh_putih,thresh_putih,getStructuringElement(MORPH_RECT,Size(1,kernel_vtl)));
 dilate(thresh_putih,thresh_putih,getStructuringElement(MORPH_RECT,Size(1,kernel_vtl)));
 vector<vector<Point > > contours;
 vector<Vec4i > hierarchy;
 findContours(thresh_putih,contours,hierarchy,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);
 vector<Rect> daerah_kontur(contours.size());
 for(size_t i=0;i<contours.size();i++){
     daerah_kontur[i] = boundingRect(contours[i]);
 }

 int hitung=0;
 int idx_kontur=0;
 for(size_t i=0;i<kandidat_tiang.size()&&hitung<2;i++){
     for(size_t j=0;j<daerah_kontur.size()&&hitung<2;j++){
         if(kandidat_tiang[i].x>daerah_kontur[j].tl().x&&kandidat_tiang[i].x<daerah_kontur[j].br().x&&
                 kandidat_tiang[i].y>daerah_kontur[j].tl().y&&kandidat_tiang[i].y<daerah_kontur[j].br().y){
             RotatedRect rr = minAreaRect(contours[j]);
             Point2f pojok[4];
             rr.points(pojok);
             line(img2,pojok[0],pojok[1],Scalar(0,0,255),2);
             line(img2,pojok[1],pojok[2],Scalar(0,0,255),2);
             line(img2,pojok[2],pojok[3],Scalar(0,0,255),2);
             line(img2,pojok[3],pojok[0],Scalar(0,0,255),2);
             hitung++;
             idx_kontur=j;
         }
     }
 }*/

