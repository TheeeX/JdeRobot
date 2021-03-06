
#include <jderobot/camera.h>

#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace kinect{
    pthread_mutex_t mutexDepth;
    cv::Mat imageDepth;
    class CamaraDepth: virtual public jderobot::Camera {
	    public:
		    CamaraDepth(std::string& propertyPrefix) : prefix(propertyPrefix) {
		       
                imageDescription = (new jderobot::ImageDescription());
                //cameraDescription = (new jderobot::CameraDescription());

                replyTask = new ReplyTask(this);
                replyTask->start(); // my own thread
		    }

		    std::string getName () {
			    return (cameraDescription->name);
		    }

		    std::string getRobotName () {
			    return ("");
		    }

		    virtual ~CamaraDepth() {

		    }

		    virtual jderobot::ImageDescriptionPtr getImageDescription(const Ice::Current& c){
			    return imageDescription;
		    }

		    virtual jderobot::CameraDescriptionPtr getCameraDescription(const Ice::Current& c){
			    return cameraDescription;
		    }

		    virtual Ice::Int setCameraDescription(const jderobot::CameraDescriptionPtr &description, const Ice::Current& c) {
			    return 0;
		    }

		    virtual void getImageData_async(const jderobot::AMD_ImageProvider_getImageDataPtr& cb,const Ice::Current& c){
			    replyTask->pushJob(cb);
		    }

		    virtual std::string startCameraStreaming(const Ice::Current&){

		    }

		    virtual void stopCameraStreaming(const Ice::Current&) {

		    }

			virtual void reset(const Ice::Current&)
			{
			}

	    private:
		    class ReplyTask:  public IceUtil::Thread {
			    public:
				    ReplyTask(CamaraDepth* camera):  mycamera(camera) {
				    }

				    void pushJob(const jderobot::AMD_ImageProvider_getImageDataPtr& cb){
					    IceUtil::Mutex::Lock sync(requestsMutex);
					    requests.push_back(cb);
				    }
                 

				    virtual void run(){
                        cv::Mat image;

                        while(!imageDepth.data)
                            usleep(1000);

                        if(imageDepth.data){
                            pthread_mutex_lock(&mutexDepth);
                            image = imageDepth.clone();
                            pthread_mutex_unlock(&mutexDepth);
                            mycamera->imageDescription->width = image.cols;
                            mycamera->imageDescription->height = image.rows;
                            mycamera->imageDescription->size = image.cols*image.rows*3;
                            mycamera->imageDescription->format = "RGB8";
                        }
                   
				
					    jderobot::ImageDataPtr reply(new jderobot::ImageData);
					    reply->description = mycamera->imageDescription;
					    struct timeval a, b;
					    int cycle = 50;
					    long totalb,totala;
					    long diff;

                        while(1){					
                            gettimeofday(&a,NULL);
                            totala=a.tv_sec*1000000+a.tv_usec;

                            IceUtil::Time t = IceUtil::Time::now();
                            reply->timeStamp.seconds = (long)t.toSeconds();
                            reply->timeStamp.useconds = (long)t.toMicroSeconds() - reply->timeStamp.seconds*1000000;

                            reply->pixelData.resize(image.rows*image.cols*3);

                            pthread_mutex_lock(&mutexDepth);
                            cv::Mat image = imageDepth.clone();
                            pthread_mutex_unlock(&mutexDepth);

                            memcpy( &(reply->pixelData[0]), (unsigned char *) image.data, image.rows*image.cols*3);

                            { //critical region start
                                IceUtil::Mutex::Lock sync(requestsMutex);
                                while(!requests.empty()) {
                                    jderobot::AMD_ImageProvider_getImageDataPtr cb = requests.front();
                                    requests.pop_front();
                                    cb->ice_response(reply);
                                }
                            } //critical region end
                      
						    gettimeofday(&b,NULL);
						    totalb=b.tv_sec*1000000+b.tv_usec;

						    diff = (totalb-totala)/1000;
						    diff = cycle-diff;

						    //std::cout << "Gazeboserver takes " << diff << " ms " << mycamera->fileName << std::endl;

						    if(diff < 33)
							    diff = 33;
						    //Sleep Algorithm//
						    usleep(diff*1000);
					    }
					
				    }
				
				
				    CamaraDepth* mycamera;
				    IceUtil::Mutex requestsMutex;
				    std::list<jderobot::AMD_ImageProvider_getImageDataPtr> requests;
		    };

		    typedef IceUtil::Handle<ReplyTask> ReplyTaskPtr;
		    std::string prefix;
		    jderobot::ImageDescriptionPtr imageDescription;
		    jderobot::CameraDescriptionPtr cameraDescription;
		    ReplyTaskPtr replyTask;

	    }; // end class CamaraDepth
}
