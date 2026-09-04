#pragma once

#include "DicomLoadService.h"
#include "LoadWorker.h"
#include <QThread>

DicomLoadService::DicomLoadService(QObject* parent) : QObject(parent) {}
DicomLoadService::~DicomLoadService() {}

void DicomLoadService::loadAsync(const QString& folderPath) {
    QThread* thread = new QThread(this);
    LoaderWorker* worker = new LoaderWorker(folderPath);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &LoaderWorker::run);
    connect(worker, &LoaderWorker::finished, this, [=](vtkSmartPointer<vtkImageData> data) {
        emit finished(data);
        thread->quit();
        });
    connect(worker, &LoaderWorker::errorOccurred, this, [=](QString msg) {
        emit error(msg);
        thread->quit();
        });

    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    thread->start();
}


//connect(thread, &QThread::started, worker, &LoaderWorker::run);
//
//스레드가 실제로 구동(start())되면, 자동으로 워커의 run() 함수가 실행되도록 연결합니다.
//
//## connect(worker, &LoaderWorker::finished, ...); &errorOccurred
//
//워커가 작업을 성공(finished)하거나 실패(errorOccurred)했을 때, 결과를 외부로 전달(emit)하고 스레드를 멈추는(thread->quit()) 동작을 연결합니다.
//
//connect(thread, &QThread::finished, ... deleteLater); (정리 예약)
//
//worker->deleteLater: 스레드가 완전히 끝났을 때 워커 메모리를 자동 해제합니다.
//
//thread->deleteLater : 스레드 객체 자신도 자동 해제합니다.
//
//this->deleteLater : (현재 코드를 품고 있는 서비스 객체 등) 작업이 끝난 주체도 안전하게 해제되도록 예약합니다.