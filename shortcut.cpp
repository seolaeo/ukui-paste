#include <QDebug>

#include "shortcut.h"

EventMonitor::EventMonitor(QObject *parent) : QThread(parent),
	m_isPress(false),
	m_display(nullptr)
{
}

void EventMonitor::run(void)
{
	Display *display = XOpenDisplay(nullptr);
	XRecordClientSpec clients = XRecordAllClients;
	XRecordRange *range = XRecordAllocRange();

	memset(range, 0, sizeof(XRecordRange));
	range->device_events.first = KeyPress;
	range->device_events.last = KeyPress;

	m_context = XRecordCreateContext(display, 0, &clients, 1, &range, 1);
	XFree(range);
	XSync(display, True);

	this->m_display = XOpenDisplay(nullptr);
	XRecordEnableContext(this->m_display, m_context, &EventMonitor::callback, reinterpret_cast<XPointer>(this));
}

void EventMonitor::stop()
{
	XRecordDisableContext(this->m_display, this->m_context);
	XFlush(this->m_display);
}

void EventMonitor::callback(XPointer ptr, XRecordInterceptData *data)
{
	reinterpret_cast<EventMonitor*>(ptr)->handleRecordEvent(data);
}

void EventMonitor::handleRecordEvent(XRecordInterceptData *data)
{
	if (data->category == XRecordFromServer) {
		xEvent *event = reinterpret_cast<xEvent*>(data->data);
		switch (event->u.u.type) {
		case KeyPress:
			emit keyPress(static_cast<unsigned char*>(data->data)[1]);
			qDebug() << (data->data)[1] << (int)Qt::Key_Control;
			break;
		default:
			break;
		}
	}

	XRecordFreeData(data);
}

Shortcut::Shortcut(QObject *parent) : QObject(parent),
	m_timer(new QTimer)
{
	auto slot_onKeyPressed = [this](void) {
		if (this->m_isActive) {
			emit this->activated();
		}

		this->m_isActive = true;
		if (this->m_timer->isActive())
			this->m_timer->stop();

		/* Record Press */
		this->m_timer->start(400);
	};
	this->m_eventMonitor = new EventMonitor(this);
	QObject::connect(this->m_eventMonitor, &EventMonitor::keyPress, this, slot_onKeyPressed);
	QObject::connect(this->m_timer, &QTimer::timeout, [this](void) {
		this->m_isActive = false;
	});
	this->m_eventMonitor->start();
}

Shortcut::~Shortcut()
{
	this->m_eventMonitor->stop();
	this->m_eventMonitor->deleteLater();
}
